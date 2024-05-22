#include "kovsh.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#define STATIC_ARR_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

// PRIVATE FUNCTION DECLARATIONS
static void lexer_inc(Lexer *l, size_t count);
static void lexer_trim(Lexer *l);
static bool is_lit(int letter);
static bool is_dig(int s) { return isdigit(s); }

static bool make_string_token(StrView sv, Token *out);
static bool make_static_token(StrView sv, Token *out);
static bool make_variety_token(StrView sv, Token *out);

static bool token_type_expect_arg_val_type(TokenType tt, ArgValType avt);

static const struct {
    TokenType type;
    StrView text;
} static_tokens[] = {
    { .type = TOKEN_TYPE_EQ, .text = STRV_LIT("=") },
    { .type = TOKEN_TYPE_BOOL, .text = STRV_LIT("true") },
    { .type = TOKEN_TYPE_BOOL, .text = STRV_LIT("false") },
    { .type = TOKEN_TYPE_PLUS, .text = STRV_LIT("+") }
};

static const struct {
    TokenType type;
    bool (*check_fn)(int s);
} variety_tokens[] = {
    { .type = TOKEN_TYPE_NUMBER, .check_fn = is_dig },
    { .type = TOKEN_TYPE_LIT, .check_fn = is_lit },
};

static KshErr cmd_eval_fn(Parser *p, CommandCall *ctx);
static KshErr args_eval_fn(Parser *p, CommandCall *ctx);

typedef KshErr (*CmdCallWorkflowFn)(Parser *p, CommandCall *cmd_call);
static const CmdCallWorkflowFn cmd_call_workflow[] = {
    cmd_eval_fn,
    args_eval_fn
};

// PUBLIC FUNCTIONS
const char *ksh_lexer_token_type_to_string(TokenType tt)
{
    switch (tt) {
    case TOKEN_TYPE_LIT: return "lit";
    case TOKEN_TYPE_STRING: return "string";
    case TOKEN_TYPE_NUMBER: return "number";
    case TOKEN_TYPE_BOOL: return "bool";
    case TOKEN_TYPE_EQ: return "eq";
    case TOKEN_TYPE_INVALID: return "invalid";
    default: return "unknown";
    }
}

bool ksh_lexer_peek_token(Lexer *l, Token *t)
{
    if (l->buf.text.items) {
        *t = l->buf;
        return true;
    }

    lexer_trim(l);
    if (l->text.items[l->cursor] == '\0') return false;

    Token tok;
    StrView text = { .items = &l->text.items[l->cursor], .len = l->text.len - l->cursor };
    if (!make_variety_token(text, &tok)
        && !make_static_token(text, &tok)
        && !make_string_token(text, &tok)) return false;
    l->buf = tok;
    *t = tok;

    return true;
}

Lexer ksh_lexer_new(StrView ss)
{
    return (Lexer) { .text = ss };
}

bool ksh_lexer_next_token(Lexer *l, Token *out)
{
    if (l->text.items[l->cursor] == '\0'
        || !ksh_lexer_peek_token(l, out)) return false;
    lexer_inc(l, out->text.len);
    l->buf = (Token){0};
    return true;
}

bool ksh_lexer_is_next_token(Lexer *l, TokenType tt)
{
    Token tok;
    if (ksh_lexer_peek_token(l, &tok)) {
        return tok.type == tt;
    }

    return false;
}

bool ksh_lexer_next_token_if(Lexer *l, TokenType tt, Token *t)
{
    if (ksh_lexer_peek_token(l, t) &&
        t->type == tt) {
        lexer_inc(l, t->text.len);
        l->buf = (Token){0};
        return true;
    }

    return false;
}

KshErr ksh_lexer_expect_next_token(Lexer *l, TokenType expect, Token *out)
{
    if (!ksh_lexer_next_token(l, out)
        || out->type != expect) {
        KSH_LOG_ERR("token_expected: %s was expected",
                    ksh_lexer_token_type_to_string(expect));
        return KSH_ERR_TOKEN_EXPECTED;
    }

    return KSH_ERR_OK;
}

ArgVal ksh_token_to_arg_val(Token tok)
{
    ArgVal result = {0};
    switch (tok.type) {
    case TOKEN_TYPE_STRING:
        result.as_str = strv_new(&tok.text.items[1], tok.text.len-2);
        break;
    case TOKEN_TYPE_LIT:
        result.as_str = tok.text;
        break;
    case TOKEN_TYPE_NUMBER:;
        char *buf = malloc(tok.text.len);
        memcpy(buf, tok.text.items, tok.text.len);
        result.as_int = atoi(buf);
        free(buf);
        break;
    case TOKEN_TYPE_BOOL:
        result.as_bool = tok.text.items[0] == 't' ? true : false;
        break;
    default: assert(0 && "not yet implemented");
    }

    return result;
}

KshErr ksh_parse(Parser *p)
{
    KshErr err;
    CommandCall cmd_call;
    for (;;) {
        for (size_t i = 0; i < STATIC_ARR_LEN(cmd_call_workflow); i++) {
            err = cmd_call_workflow[i](p, &cmd_call);
            if (err != KSH_ERR_OK) return err;
        }

        err = ksh_cmd_call_exec(cmd_call);
        if (err != KSH_ERR_OK) return err;

        Token tok;
        if (!ksh_lexer_peek_token(&p->lex, &tok)
            || tok.type != TOKEN_TYPE_PLUS) {
            break;
        }

        ksh_lexer_next_token(&p->lex, &tok);
    }

    return KSH_ERR_OK;
}

// bool ksh_parser_parse_cmd_call(Parser *p, CommandCall *cmd_call, KshErr *err)
// {
//     Token tok;
// 
//     *err = ksh_lexer_expect_next_token(p->lex, TOKEN_TYPE_LIT, &tok);
//     if (err != KSH_ERR_OK) return false;
// 
//     Command *cmd = ksh_cmd_find(p->info.cmds, tok.text);
//     if (cmd == NULL) {
//         KSH_LOG_ERR("command not found: `"STRV_FMT"`", STRV_ARG(tok.text));
//         *err = KSH_ERR_COMMAND_NOT_FOUND;
//         return false;
//     }
// 
//     CommandCall cmd_call = ksh_cmd_create_call(cmd);
//     while (ksh_parser_parse_arg(p, cmd_call, err));
//     if (*err != KSH_ERR_OK) return false;
// }
// 
// bool ksh_parser_parse_arg(Parser *p, CommandCall *cmd_call, KshErr *err)
// {
//     Token tok;
//     if (!ksh_lexer_next_token(p->lex, &tok)) return false;
//     
//     if (ksh_lexer_peek_token(p->lex, &tok)
//         && tok.type == TOKEN_TYPE_EQ) {
//     } else {
//         *err = ksh_token_parse_to_arg(tok, cmd_call->argv[0]);
//         if (*err != KSH_ERR_OK) return false;
//     }
// 
//     if (!ksh_lexer_next_token(p->lex, &tok)) return false;
// 
// }

// PRIVATE FUNCTIONS
static bool make_static_token(StrView sv, Token *out)
{
    for (size_t i = 0; i < STATIC_ARR_LEN(static_tokens); i++) {
        if (sv.len < static_tokens[i].text.len) continue;
        StrView sv2 = { .items = sv.items, .len = static_tokens[i].text.len };
        if (strv_eq(sv2, static_tokens[i].text)) {
            *out = (Token){
                .text = sv2,
                .type = static_tokens[i].type
            };
            return true;
        }
    }

    return false;
}

static bool make_variety_token(StrView sv, Token *out)
{
    for (size_t i = 0; i < STATIC_ARR_LEN(variety_tokens); i++) {
        bool (*check_fn)(int s) = variety_tokens[i].check_fn; 
        if (!check_fn(sv.items[0])) continue;
        *out = (Token){
            .text.items = sv.items,
            .text.len = 1,
            .type = variety_tokens[i].type
        };

        size_t i;
        for (i = 1; check_fn(sv.items[i]) && i < sv.len; i++) {
            out->text.len++;
        }

        return true;
    }

    return false;
}

static bool make_string_token(StrView sv, Token *out)
{
    if (sv.items[0] != '"') return false;
    for (size_t i = 1; i < sv.len; i++) {
        if (sv.items[i] == '"') {
            *out = (Token){
                .text.items = sv.items,
                .text.len = i+1,
                .type = TOKEN_TYPE_STRING
            };
            return true;
        }
    }

    return false;
}

static void lexer_inc(Lexer *l, size_t inc)
{
    assert(l->cursor + inc <= l->text.len);
    l->cursor += inc;
}

static void lexer_trim(Lexer *self) {
    while (self->text.items[self->cursor] == ' ') { self->cursor++;
    }
}

static bool is_lit(int letter)
{
    return ('a' <= letter && letter <= 'z')
           || ('A' <= letter && letter <= 'Z')
           || ('0' <= letter && letter <= '9');
}

static bool token_type_expect_arg_val_type(TokenType tt, ArgValType avt)
{
    switch (tt) {
    case TOKEN_TYPE_STRING:
    case TOKEN_TYPE_LIT:
        if (avt != ARG_VAL_TYPE_STR) return false;
        break;
    case TOKEN_TYPE_NUMBER:
        if (avt != ARG_VAL_TYPE_INT) return false;
        break;
    case TOKEN_TYPE_BOOL:
        if (avt != ARG_VAL_TYPE_INT) return false;
        break;
    default: return false;
    }

    return true;
}

static KshErr cmd_eval_fn(Parser *p, CommandCall *ctx)
{
    Token tok;
    KshErr err = ksh_lexer_expect_next_token(&p->lex, TOKEN_TYPE_LIT, &tok);
    if (err != KSH_ERR_OK) return err;

    Command *cmd = ksh_cmd_find(p->info.cmds, tok.text);
    if (cmd == NULL) {
        KSH_LOG_ERR("command not found: `"STRV_FMT"`", STRV_ARG(tok.text));
        return KSH_ERR_COMMAND_NOT_FOUND;
    }

    *ctx = ksh_cmd_create_call(cmd);

    return KSH_ERR_OK;
}

// TODO: refactor this shit!!!
static KshErr args_eval_fn(Parser *p, CommandCall *ctx)
{
    assert(ctx->cmd);

    Arg *arg;
    Token arg_name;
    Token arg_val;
    while (ksh_lexer_peek_token(&p->lex, &arg_name) &&
           arg_name.type != TOKEN_TYPE_PLUS) {
        ksh_lexer_next_token(&p->lex, &arg_name);
        if (arg_name.type == TOKEN_TYPE_LIT &&
            ksh_lexer_next_token_if(&p->lex, TOKEN_TYPE_EQ, &arg_val) &&
            ksh_lexer_next_token(&p->lex, &arg_val))
        {
            arg = ksh_args_find(ctx->argc, ctx->argv, arg_name.text);
            ctx->last_assigned = (arg - ctx->argv) / sizeof(*arg);
        } else {
            if (ctx->last_assigned >= ctx->argc) {
                KSH_LOG_ERR("last arg not found%s", "");
                return KSH_ERR_ARG_NOT_FOUND;
            }
            arg_val = arg_name;
            arg = &ctx->argv[ctx->last_assigned++];
        }

        if (arg == NULL) {
            KSH_LOG_ERR("arg not found: `"STRV_FMT"`", STRV_ARG(arg_name.text));
            return KSH_ERR_ARG_NOT_FOUND;
        }

        if (!token_type_expect_arg_val_type(arg_val.type, arg->def->type)) {
            KSH_LOG_ERR("arg `"STRV_FMT"`: expected type: `%s`",
                        STRV_ARG(arg->def->name),
                        ksh_arg_val_type_to_str(arg->def->type));
            return KSH_ERR_TYPE_EXPECTED;
        }

        arg->value = ksh_token_to_arg_val(arg_val);
        arg->is_assign = true;
    }

    return KSH_ERR_OK;
}
