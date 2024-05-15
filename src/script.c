#include "kovsh.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

typedef Token (*TokenMakeFn)(Lexer *);

static Token token_lit_make_fn(Lexer *);
static Token token_string_make_fn(Lexer *);
static Token token_number_make_fn(Lexer *);
static Token token_one_letter_make_fn(Lexer *l);

static bool token_type_expect_arg_val_type(TokenType tt, ArgValType avt);

static const TokenMakeFn TOKEN_MAKE_FUNCTIONS[] = {
    [TOKEN_TYPE_LIT] = token_lit_make_fn,
    [TOKEN_TYPE_STRING] = token_string_make_fn,
    [TOKEN_TYPE_NUMBER] = token_number_make_fn,
    [TOKEN_TYPE_BOOL] = token_lit_make_fn,
    [TOKEN_TYPE_EQ] = token_one_letter_make_fn
};

// KEYWORDS
const StrView BOOL_TRUE_KEYWORD = { .items = "true", .len = 4 };
const StrView BOOL_FALSE_KEYWORD = { .items = "false", .len = 5 };

// PRIVATE FUNCTION DECLARATIONS
static void lexer_trim(Lexer *l);
static bool is_lit(int letter);
static bool lexer_at_keyword(Lexer *l, StrView keyword);

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

Lexer ksh_lexer_new(StrView ss)
{
    return (Lexer) { .text = ss };
}

TokenType ksh_lexer_compute_token_type(Lexer *l)
{
    lexer_trim(l);
    int letter = l->text.items[l->cursor];

    switch (letter) {
    case '=': return TOKEN_TYPE_EQ;
    case '"': return TOKEN_TYPE_STRING;

    case '\n':
    case '\0': return TOKEN_TYPE_END;
    }

    if (lexer_at_keyword(l, BOOL_TRUE_KEYWORD)
        || lexer_at_keyword(l, BOOL_FALSE_KEYWORD)) return TOKEN_TYPE_BOOL;
    if (isdigit(letter)) return TOKEN_TYPE_NUMBER;
    if (is_lit(letter)) return TOKEN_TYPE_LIT;

    return TOKEN_TYPE_INVALID;
}

Token ksh_lexer_make_token(Lexer *l, TokenType tt)
{
    assert(tt < TOKEN_TYPE_INVALID);
    Token token = TOKEN_MAKE_FUNCTIONS[tt](l);
    token.type = tt;
    return token;
}

Token ksh_lexer_next_token(Lexer *l)
{
    TokenType tt = ksh_lexer_compute_token_type(l);

    Token t;
    if (tt < TOKEN_TYPE_INVALID) {
        t = ksh_lexer_make_token(l, tt);
        ksh_lexer_inc(l, t.text.len);
    } else {
        t = (Token){ .type = tt };
    }
    return t;
}

void ksh_lexer_inc(Lexer *l, size_t inc)
{
    assert(l->cursor + inc <= l->text.len);
    l->cursor += inc;
}

KshErr ksh_lexer_expect_next_token(Lexer *l, TokenType expect, Token *out)
{
    TokenType tt = ksh_lexer_compute_token_type(l);
    if (tt != expect) {
        KSH_LOG_ERR("token_expected: %s was expected but %s was occured",
                    ksh_lexer_token_type_to_string(expect),
                    ksh_lexer_token_type_to_string(tt));
        return KSH_ERR_TOKEN_EXPECTED;
    }

    *out = ksh_lexer_make_token(l, expect);
    ksh_lexer_inc(l, out->text.len);
    return KSH_ERR_OK;
}

bool ksh_lexer_is_token_next(Lexer *l, TokenType t)
{
    return ksh_lexer_compute_token_type(l) == t;
}

KshErr ksh_token_parse_to_arg(Token token, Arg *arg)
{
    if (!token_type_expect_arg_val_type(token.type, arg->def->type)) {
        KSH_LOG_ERR("type expected: the `"STRV_FMT"` arg expected value type `%s`",
                    STRV_ARG(arg->def->name),
                    ksh_arg_val_type_to_str(arg->def->type));
        return KSH_ERR_TYPE_EXPECTED;
    }

    switch (token.type) {
    case TOKEN_TYPE_LIT:
        arg->value.as_str = token.text;
        break;
    case TOKEN_TYPE_STRING:
        arg->value.as_str = (StrView){
            .items = token.text.items+1,
            .len = token.text.len-2,
        };
        break;
    case TOKEN_TYPE_NUMBER:;
        char *buf = malloc(token.text.len);
        memcpy(buf, token.text.items, token.text.len);
        arg->value.as_int = atoi(buf);
        free(buf);
        break;
    case TOKEN_TYPE_BOOL:
        arg->value.as_bool = token.text.items[0] == 't' ? true : false;
        break;
    default:
        assert(0 && "not yet implemented");
    }

    arg->is_assign = true;
    return KSH_ERR_OK;
}

KshErr ksh_parse_lexer(CommandBuf cmd_buf, Lexer *l)
{
    KshErr err;

    Token cmd_token; 
    err = ksh_lexer_expect_next_token(l, TOKEN_TYPE_LIT, &cmd_token);
    if (err != KSH_ERR_OK) return err;

    Command *cmd = ksh_cmd_find(cmd_buf, cmd_token.text);
    if (cmd == NULL) {
        KSH_LOG_ERR("command not found: `"STRV_FMT"`", STRV_ARG(cmd_token.text));
        return KSH_ERR_COMMAND_NOT_FOUND;
    }

    CommandCall cmd_call = ksh_cmd_create_call(cmd);

    TokenType next = ksh_lexer_compute_token_type(l);
    while (next == TOKEN_TYPE_LIT) {
        Token arg_name;
        err = ksh_lexer_expect_next_token(l, TOKEN_TYPE_LIT, &arg_name);
        if (err != KSH_ERR_OK) return err;

        Arg *arg = ksh_args_find(cmd_call.argc, cmd_call.argv, arg_name.text);
        if (arg == NULL) {
            KSH_LOG_ERR("arg not found: `"STRV_FMT"`", STRV_ARG(arg_name.text));
            return KSH_ERR_ARG_NOT_FOUND;
        }

        Token eq;
        err = ksh_lexer_expect_next_token(l, TOKEN_TYPE_EQ, &eq);
        if (err != KSH_ERR_OK) return err;

        Token token = ksh_lexer_next_token(l);

        err = ksh_token_parse_to_arg(token, arg);
        if (err != KSH_ERR_OK) return err;

        next = ksh_lexer_compute_token_type(l);
    }

    err = ksh_cmd_call_exec(cmd_call);
    if (err != KSH_ERR_OK) return err;

    return KSH_ERR_OK;
}

// PRIVATE FUNCTIONS
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

static bool lexer_at_keyword(Lexer *l, StrView ss)
{
    if ((l->text.len - l->cursor == ss.len)
       || isspace(l->text.items[l->cursor+ss.len])) {
        return strncmp(&l->text.items[l->cursor], ss.items, ss.len) == 0;
    }

    return false;
}

static Token token_lit_make_fn(Lexer *l)
{
    size_t cursor = l->cursor;
    assert(is_lit(l->text.items[cursor]));
    Token result = {
        .text.items = &l->text.items[cursor++],
        .text.len = 1
    };

    while (is_lit(l->text.items[cursor])) {
        cursor++;
        result.text.len++;
    }

    return result;
}

static Token token_string_make_fn(Lexer *l)
{
    size_t cursor = l->cursor;
    assert(l->text.items[cursor] == '"');
    Token result = {
        .text.items = &l->text.items[cursor++],
        .text.len = 2
    };

    while (l->text.items[cursor] != '"') {
        if (l->text.items[cursor+1] == '\0') {
            break;
        }
        cursor++;
        result.text.len++;
    }

    return result;
}

static Token token_number_make_fn(Lexer *l)
{
    size_t cursor = l->cursor;
    assert(isdigit(l->text.items[cursor]));
    Token result = {
        .text.items = &l->text.items[cursor++],
        .text.len = 1
    };

    while (isdigit(l->text.items[cursor])) {
        cursor++;
        result.text.len++;
    }

    return result;
}

static Token token_one_letter_make_fn(Lexer *l)
{
    return (Token) {
        .text.items = &l->text.items[l->cursor],
        .text.len = 1
    };
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
