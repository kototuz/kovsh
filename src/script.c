#include "kovsh.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#define STATIC_ARR_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

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

typedef bool (*ParseFn)(StrView sv, void *ctx, Token *out);
typedef KshErr (*TokenConvertFn)(Token tok, KshValue *dest);



static void lexer_inc(Lexer *l, size_t count);
static void lexer_trim(Lexer *l);
static bool is_lit(int letter);
static bool is_dig(int s) { return isdigit(s); }

static bool parse_string_token(StrView sv, void *ctx, Token *out);
static bool parse_variety_token(StrView sv, Variety *ctx, Token *out);
static bool parse_keyword_token(StrView sv, Keyword *ctx, Token *out);
static bool parse_spec_sym_token(StrView sv, SpecialSymbol *ctx, Token *out);
static bool parse_var_token(StrView sv, void *ctx, Token *out);



static const ParseDb keyword_db = {
    .len = 2,
    .item_size = sizeof(Keyword),
    .items = (uint8_t *)(Keyword[]){
        { .type = TOKEN_TYPE_BOOL, .word = STRV_LIT("true") },
        { .type = TOKEN_TYPE_BOOL, .word = STRV_LIT("false") },
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

static const struct TokenParser {
    const ParseDb db;
    ParseFn parse_fn;
} token_parsers[] = {
    { .db = keyword_db, .parse_fn = (ParseFn) parse_keyword_token },
    { .db = special_symbol_db, .parse_fn = (ParseFn) parse_spec_sym_token },
    { .db = variety_db, .parse_fn = (ParseFn) parse_variety_token },
    { .parse_fn = (ParseFn) parse_string_token },
    { .parse_fn = (ParseFn) parse_var_token }
};

static const struct {
    KshValueTypeTag *tags;
    size_t tags_len;
} tok_to_val_type_map[] = {
    {
        .tags = (KshValueTypeTag[]){
            KSH_VALUE_TYPE_TAG_STR 
        },
        .tags_len = 1
    },
    {   
        .tags = (KshValueTypeTag[]){
            KSH_VALUE_TYPE_TAG_STR,
            KSH_VALUE_TYPE_TAG_ENUM 
        },
        .tags_len = 2
    },
    {
        .tags = (KshValueTypeTag[]){
            KSH_VALUE_TYPE_TAG_INT 
        },
        .tags_len = 1
    },
    {
        .tags = (KshValueTypeTag[]){
            KSH_VALUE_TYPE_TAG_BOOL
        },
        .tags_len = 1,
    },
};

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

    StrView text = { .items = &l->text.items[l->cursor], .len = l->text.len - l->cursor };
    KshErr err = ksh_token_from_strv(text, t);
    if (err != KSH_ERR_OK) return false;
    l->buf = *t;

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
        || out->type != expect) return KSH_ERR_TOKEN_EXPECTED;

    return KSH_ERR_OK;
}

KshErr ksh_token_actual(Token *tok)
{
    KshErr err;
    StrView text;
    switch (tok->type) {
        case TOKEN_TYPE_STRING:
            tok->text.items += 1;
            tok->text.len -= 2;
            return KSH_ERR_OK;
        case TOKEN_TYPE_VAR:
            err = ksh_var_get_val((StrView){
                .items = &tok->text.items[1],
                .len = tok->text.len-1
            }, &text);
            if (err != KSH_ERR_OK) return err;
            err = ksh_token_from_strv(text, tok);
            if (err != KSH_ERR_OK) return err;
            return ksh_token_actual(tok);
        default: break;
    }

    return KSH_ERR_OK;
}

KshErr ksh_token_get_actual_data(Token tok, StrView *dest)
{
    KshErr err;
    switch (tok.type) {
        case TOKEN_TYPE_STRING:
            dest->items = &tok.text.items[1];
            dest->len = tok.text.len-2;
            break;
        case TOKEN_TYPE_VAR:
            err = ksh_var_get_val((StrView){
                .items = &tok.text.items[1],
                .len = tok.text.len-1
            }, dest);
            if (err != KSH_ERR_OK) return err;
            break;
        default: *dest = tok.text;
    }

    return KSH_ERR_OK;
}

bool ksh_token_type_fit_value_type(TokenType tt, KshValueTypeTag val_t)
{
    if (tt == TOKEN_TYPE_VAR) return true;
    for (size_t i = 0; i < STATIC_ARR_LEN(tok_to_val_type_map); i++)
        for (size_t j = 0; j < tok_to_val_type_map[i].tags_len; j++)
            if (tok_to_val_type_map[i].tags[j] == val_t)
                return true;

    return false;
}

KshErr ksh_token_from_strv(StrView sv, Token *dest)
{
    for (size_t i = 0; i < STATIC_ARR_LEN(token_parsers); i++) {
        struct TokenParser tp = token_parsers[i];

        if (tp.db.len == 0)
            if (tp.parse_fn(sv, NULL, dest))
                return KSH_ERR_OK;

        for (size_t i = 0; i < tp.db.len; i++)
            if (tp.parse_fn(sv, &tp.db.items[i*tp.db.item_size], dest))
                return KSH_ERR_OK;
    }

    return KSH_ERR_UNDEFINED_TOKEN;
}

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

static bool parse_var_token(StrView sv, void *ctx, Token *out)
{
    (void) ctx;
    if (sv.items[0] != '@') return false;

    if (!parse_variety_token(
            (StrView){
                .items = &sv.items[1],
                .len = sv.len-1
            },
            &(Variety){ 0, is_lit },
            out
        )) return false;

    out->text.items = sv.items;
    out->text.len++;
    out->type = TOKEN_TYPE_VAR;

    return true;
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
