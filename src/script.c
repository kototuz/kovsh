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

static const TokenMakeFn TOKEN_MAKE_FUNCTIONS[] = {
    [TOKEN_TYPE_LIT] = token_lit_make_fn,
    [TOKEN_TYPE_STRING] = token_string_make_fn,
    [TOKEN_TYPE_NUMBER] = token_number_make_fn,
    [TOKEN_TYPE_BOOL] = token_lit_make_fn,
    [TOKEN_TYPE_EQ] = token_one_letter_make_fn
};

// KEYWORDS
const StrSlice BOOL_TRUE_KEYWORD = { .items = "true", .len = 4 };
const StrSlice BOOL_FALSE_KEYWORD = { .items = "false", .len = 5 };

// PRIVATE FUNCTION DECLARATIONS
static void lexer_trim(Lexer *l);
static bool is_lit(int letter);
static bool lexer_at_keyword(Lexer *l, StrSlice keyword);

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

Lexer ksh_lexer_new(StrSlice ss)
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
    l->cursor += inc;
}

Token ksh_lexer_expect_next_token(Lexer *l, TokenType expect)
{
    TokenType tt = ksh_lexer_compute_token_type(l);
    if (tt != expect) {
        fprintf(stderr, "ERROR: %s was expected but %s is occured\n",
                ksh_lexer_token_type_to_string(expect),
                ksh_lexer_token_type_to_string(tt));
        exit(1);
    }

    Token token = ksh_lexer_make_token(l, expect);
    ksh_lexer_inc(l, token.text.len);
    return token;
}

bool ksh_lexer_is_token_next(Lexer *l, TokenType t)
{
    return ksh_lexer_compute_token_type(l) == t;
}

CommandArgVal ksh_token_parse_to_arg_val(Token token)
{
    CommandArgVal result = {0};
    switch (token.type) {
    case TOKEN_TYPE_LIT:
        result.kind = COMMAND_ARG_VAL_KIND_STR;
        result.as_str = token.text;
        break;
    case TOKEN_TYPE_STRING:
        result.kind = COMMAND_ARG_VAL_KIND_STR;
        result.as_str = (StrSlice){
            .items = token.text.items+1,
            .len = token.text.len-2,
        };
        break;
    case TOKEN_TYPE_NUMBER:
        result.kind = COMMAND_ARG_VAL_KIND_INT;
        char *buf = malloc(token.text.len);
        memcpy(buf, token.text.items, token.text.len);
        result.as_int = atoi(buf);
        free(buf);
        break;
    case TOKEN_TYPE_BOOL:
        result.kind = COMMAND_ARG_VAL_KIND_BOOL;
        result.as_bool = token.text.items[0] == 't' ? true : false;
        break;
    default:
        fprintf(stderr, "ERROR: ksh_token_parse_to_arg_val: %s not yet implemented",
              ksh_lexer_token_type_to_string(token.type));
        exit(1);
    }

    return result;
}

void ksh_parse_lexer(Lexer *l)
{
    StrSlice cmd_name = (ksh_lexer_expect_next_token(l, TOKEN_TYPE_LIT)).text;
    Command *cmd = ksh_command_find(cmd_name);
    if (cmd == NULL) {
        fputs("ERROR: command ", stderr);
        strslice_print(cmd_name, stderr);
        fputs(" not found\n", stderr);
    }

    CommandCall cmd_call = ksh_command_create_call(cmd);

    TokenType next = ksh_lexer_compute_token_type(l);
    while (next == TOKEN_TYPE_LIT) {
        StrSlice arg_name = (ksh_lexer_expect_next_token(l, next)).text;

        ksh_lexer_expect_next_token(l, TOKEN_TYPE_EQ);
        Token token = ksh_lexer_next_token(l);
        CommandArgVal arg_value = ksh_token_parse_to_arg_val(token);

        ksh_commandcall_set_arg(&cmd_call, arg_name, arg_value);

        next = ksh_lexer_compute_token_type(l);
    }

    ksh_commandcall_exec(cmd_call);
}

// PRIVATE FUNCTIONS
static void lexer_trim(Lexer *self) {
    while (self->text.items[self->cursor] == ' ') { self->cursor++;
    }
}

static bool is_lit(int letter)
{
    return ('a' <= letter && letter <= 'z')
           || ('0' <= letter && letter <= '9');
}

static bool lexer_at_keyword(Lexer *l, StrSlice ss)
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
        if (l->text.items[cursor] == '\0') {
            fputs("ERROR: the closing qoute was missed\n", stderr);
            exit(1);
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
