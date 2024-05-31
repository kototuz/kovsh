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

static bool token_type_expect_arg_val_type(TokenType tt, ArgValType avt);

typedef struct {
    size_t len;
    size_t item_size;
    uint8_t *items;
} ParseDb;

typedef struct {
    TokenType type;
    StrView word;
} Keyword;

typedef struct {
    TokenType type;
    const char symbol;
} SpecialSymbol;

typedef struct {
    TokenType type;
    bool (*check_fn)(int s);
} Variety;

static const ParseDb keyword_db = {
    .len = 3,
    .item_size = sizeof(Keyword),
    .items = (uint8_t *)(Keyword[]){
        { .type = TOKEN_TYPE_BOOL, .word = STRV_LIT("true") },
        { .type = TOKEN_TYPE_BOOL, .word = STRV_LIT("false") },
        { .type = TOKEN_TYPE_KEYWORD_SYS, .word = STRV_LIT("sys") }
    },
};

static const ParseDb special_symbol_db = {
    .len = 2,
    .item_size = sizeof(SpecialSymbol),
    .items = (uint8_t *)(SpecialSymbol[]){
        { .type = TOKEN_TYPE_EQ, .symbol = '=' },
        { .type = TOKEN_TYPE_PLUS, .symbol = '+' }
    }
};

static const ParseDb variety_db = {
    .len = 2,
    .item_size = sizeof(Variety),
    .items = (uint8_t *)(Variety[]){
        { .type = TOKEN_TYPE_NUMBER, .check_fn = is_dig },
        { .type = TOKEN_TYPE_LIT, .check_fn = is_lit }
    }
};

static bool parse_string_token(StrView sv, void *ctx, Token *out);
static bool parse_variety_token(StrView sv, Variety *ctx, Token *out);
static bool parse_keyword_token(StrView sv, Keyword *ctx, Token *out);
static bool parse_spec_sym_token(StrView sv, SpecialSymbol *ctx, Token *out);

typedef bool (*ParseFn)(StrView sv, void *ctx, Token *out);

static const struct TokenParser {
    const ParseDb db;
    ParseFn parse_fn;
} token_parsers[] = {
    { .db = keyword_db, .parse_fn = (ParseFn) parse_keyword_token },
    { .db = special_symbol_db, .parse_fn = (ParseFn) parse_spec_sym_token },
    { .db = variety_db, .parse_fn = (ParseFn) parse_variety_token },
    { .parse_fn = (ParseFn) parse_string_token }
};

static KshErr mod_eval_fn(Lexer *lex, Terminal *term, bool *exit);
static KshErr cmd_eval_fn(Lexer *lex, Terminal *term, bool *exit);
static KshErr args_eval_fn(Lexer *lex, Terminal *term, bool *exit);

typedef KshErr (*CmdCallWorkflowFn)(Lexer *l, Terminal *term, bool *exit);

static const CmdCallWorkflowFn cmd_call_workflow[] = {
    mod_eval_fn,
    cmd_eval_fn,
    args_eval_fn,
};

// PUBLIC FUNCTIONS
const char *ksh_lexer_token_type_to_string(TokenType tt)
{
    switch (tt) {
    case TOKEN_TYPE_LIT: return "lit";
    case TOKEN_TYPE_STRING: return "string";
    case TOKEN_TYPE_NUMBER: return "number"; case TOKEN_TYPE_BOOL: return "bool";
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
    for (size_t i = 0; i < STATIC_ARR_LEN(token_parsers); i++) {
        struct TokenParser tp = token_parsers[i];

        if (tp.db.len == 0) {
            if (tp.parse_fn(text, NULL, &tok)) {
                l->buf = tok;
                *t = tok;
                return true;
            }
        }

        for (size_t i = 0; i < tp.db.len; i++) {
            if (tp.parse_fn(text, &tp.db.items[i*tp.db.item_size], &tok)) {
                l->buf = tok;
                *t = tok;
                return true;
            }
        }
    }

    return false;
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

KshErr ksh_parse(Lexer *lex, Terminal *term)
{
    KshErr err;
    for (;;) {
        bool exit = false;
        for (size_t i = 0; i < STATIC_ARR_LEN(cmd_call_workflow); i++) {
            err = cmd_call_workflow[i](lex, term, &exit);
            if (err != KSH_ERR_OK) return err;
            if (exit) return KSH_ERR_OK;
        }

        err = ksh_cmd_call_exec(term->cur_cmd_call);
        if (err != KSH_ERR_OK) return err;

        Token tok;
        if (!ksh_lexer_peek_token(lex, &tok)
            || tok.type != TOKEN_TYPE_PLUS) {
            break;
        }

        ksh_lexer_next_token(lex, &tok);
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
static bool parse_spec_sym_token(StrView sv, SpecialSymbol *spec_sym, Token *out)
{
    if (spec_sym->symbol == sv.items[0]) {
        *out = (Token){
            .text.items = sv.items,
            .text.len = 1,
            .type = spec_sym->type
        };
        return true;
    }

    return false;
}

static bool parse_keyword_token(StrView sv, Keyword *keyword, Token *out)
{
    StrView keyword_sv = keyword->word;
    if (sv.items[keyword_sv.len] != ' ' &&
        sv.items[keyword_sv.len] != '\n') return false;

    StrView sv_cat = { .items = sv.items, .len = keyword_sv.len };
    if (strv_eq(sv_cat, keyword_sv)) {
        *out = (Token){
            .text = keyword_sv,
            .type = keyword->type,
        };
        return true;
    }

    return false;
}

static bool parse_variety_token(StrView sv, Variety *vari, Token *out)
{
    bool (*check_fn)(int s) = vari->check_fn; 
    if (!check_fn(sv.items[0])) return false;

    *out = (Token){
        .text.items = sv.items,
        .text.len = 1,
        .type = vari->type
    };

    size_t i;
    for (i = 1; check_fn(sv.items[i]) && i < sv.len; i++) {}
    out->text.len = i;

    return true;
}

static bool parse_string_token(StrView sv, void *ctx, Token *out)
{
    (void) ctx;
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

static KshErr mod_eval_fn(Lexer *l, Terminal *term, bool *exit)
{
    (void) term; (void) exit;
    Token tok;
    if (ksh_lexer_next_token_if(l, TOKEN_TYPE_KEYWORD_SYS, &tok)) {
        system(&l->text.items[3]);
    }

    return KSH_ERR_OK;
}

static KshErr cmd_eval_fn(Lexer *l, Terminal *term, bool *exit)
{
    Token tok;
    if (!ksh_lexer_peek_token(l, &tok)) {
        *exit = true;
        return KSH_ERR_OK;
    }

    KshErr err = ksh_lexer_expect_next_token(l, TOKEN_TYPE_LIT, &tok);
    if (err != KSH_ERR_OK) return err;

    Command *cmd = ksh_cmd_find(term->commands, tok.text);
    if (cmd == NULL) {
        KSH_LOG_ERR("command not found: `"STRV_FMT"`", STRV_ARG(tok.text));
        return KSH_ERR_COMMAND_NOT_FOUND;
    }

    term->cur_cmd_call = ksh_cmd_create_call(cmd);

    return KSH_ERR_OK;
}

static KshErr args_eval_fn(Lexer *lex, Terminal *term, bool *exit)
{
    (void) exit;
    assert(term->cur_cmd_call.cmd);

    Arg *arg;
    Token arg_name;
    Token arg_val;
    while (ksh_lexer_peek_token(lex, &arg_name) &&
           arg_name.type != TOKEN_TYPE_PLUS) {
        ksh_lexer_next_token(lex, &arg_name);
        if (arg_name.type == TOKEN_TYPE_LIT &&
            ksh_lexer_next_token_if(lex, TOKEN_TYPE_EQ, &arg_val) &&
            ksh_lexer_next_token(lex, &arg_val))
        {
            arg = ksh_args_find(term->cur_cmd_call.argc,
                                term->cur_cmd_call.argv,
                                arg_name.text);
            term->cur_cmd_call.last_assigned_arg_idx = arg - term->cur_cmd_call.argv + 1;
        } else {
            if (term->cur_cmd_call.last_assigned_arg_idx >= term->cur_cmd_call.argc) {
                KSH_LOG_ERR("last arg not found%s", "");
                return KSH_ERR_ARG_NOT_FOUND;
            }
            arg_val = arg_name;
            arg = &term->cur_cmd_call.argv[term->cur_cmd_call.last_assigned_arg_idx++];
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
