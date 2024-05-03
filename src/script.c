#include "kovsh.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
    TokenType tt;
    ContextUse ctx_use;
} TTPatternPair;

typedef struct {
    TTPatternPair *items;
    size_t len;
} TTPattern;

#define TOKEN_TYPE_PATTERNS_CAP (sizeof(token_type_patterns) / sizeof(TTPattern))
static const TTPattern token_type_patterns[] = {
    {
        .items = (TTPatternPair[]){
            { TOKEN_TYPE_LIT, CONTEXT_USE_ARG_NAME },
            { TOKEN_TYPE_EQ, CONTEXT_USE_NONE },
            { TOKEN_TYPE_LIT, CONTEXT_USE_ARG_VALUE }
        },
        .len = 3
    },
    {
        .items = (TTPatternPair[]){
            { TOKEN_TYPE_LIT, CONTEXT_USE_ARG_NAME },
            { TOKEN_TYPE_EQ, CONTEXT_USE_NONE },
            { TOKEN_TYPE_STRING, CONTEXT_USE_ARG_VALUE }
        },
        .len = 3
    },
    {
        .items = (TTPatternPair[]){
            { TOKEN_TYPE_LIT, CONTEXT_USE_ARG_NAME },
            { TOKEN_TYPE_EQ, CONTEXT_USE_NONE },
            { TOKEN_TYPE_NUMBER, CONTEXT_USE_ARG_VALUE }
        },
        .len = 3
    },
    {
        .items = (TTPatternPair[]){ { TOKEN_TYPE_LIT, CONTEXT_USE_CMD_CALL } },
        .len = 1
    }
};

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
static bool ksh_lexer_match_pattern(Lexer *l, TTPattern pat);

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
    l->cur_letter = l->text.items[(l->cursor += inc)];
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

TokenSeq ksh_token_seq_from_lexer(Lexer *l, size_t count)
{
    TokenSeq result = {
        .items = (Token *)malloc(sizeof(Token) * count),
        .len = count
    };

    if (!result.items) {
        fputs("ERROR: could not allocate memmory\n", stderr);
        exit(1);
    }

    for (size_t i = 0; i < count; i++) {
        Token token = ksh_lexer_next_token(l);
        if (token.type == TOKEN_TYPE_END) {
            fprintf(stderr, "ERROR: could not get %zu tokens: tokens are over", count);
            exit(1);
        }
        result.items[i] = token;
    }

    return result;
}

void ksh_context_delete(Context *context)
{
    ContextEntry *tmp;
    ContextEntry *it = context->first;
    while (it != NULL) {
        tmp = it;
        it = it->next;
        free(tmp);
    }
}

Context ksh_context_new(void)
{
    return (Context) { .first = NULL, .last = NULL };
}

ContextEntry *ksh_context_entry_create(Token t, ContextUse ctx_use) {
    ContextEntry *result = (ContextEntry *)malloc(sizeof(ContextEntry));
    if (!result) {
        fputs("ERROR: could not allocate memmory for `ContextEntry`\n", stderr);
        exit(1);
    }

    *result = (ContextEntry) {
        .token = t,
        .ctx_use = ctx_use,
        .next = NULL,
    };

    return result;
}

void ksh_context_write(Context *context, ContextEntry *ce)
{
    if (context->first == NULL) {
        context->first = context->last = ce;
    } else {
        context->last = context->last->next = ce;
    }
    context->entries_len++;
}

void ksh_context_init(Lexer *l, Context *context)
{
    for (size_t i = 0; i < TOKEN_TYPE_PATTERNS_CAP;) {
        TTPattern pattern = token_type_patterns[i];

        if (!ksh_lexer_match_pattern(l, pattern)) {
            i++;
            continue;
        }

        for (size_t y = 0; y < pattern.len; y++) {
            Token t = ksh_lexer_expect_next_token(l, pattern.items[y].tt);
            ContextEntry *entry = ksh_context_entry_create(t, pattern.items[y].ctx_use);
            ksh_context_write(context, entry);
        }
        i = 0;
    }
}

const char *ksh_context_use_to_str(ContextUse ctx_use)
{
    switch (ctx_use) {
    case CONTEXT_USE_CMD_CALL: return "cmd call";
    case CONTEXT_USE_ARG_NAME: return "arg name";
    case CONTEXT_USE_ARG_VALUE: return "arg value";
    case CONTEXT_USE_NONE: return "none";
    default: return "unknown";
    }
}

bool ksh_context_iter_next(ContextIter *ctx_iter, ContextEntry **output)
{
    if (ctx_iter->cursor == NULL) {
        if (ctx_iter->context.first == NULL) return false;
        ctx_iter->cursor = ctx_iter->context.first;
        *output = ctx_iter->cursor;
        return true;
    }

    if (ctx_iter->cursor->next == NULL) return false;
    ctx_iter->cursor = ctx_iter->cursor->next;
    *output = ctx_iter->cursor;
    return true;
}

int main(void)
{
    const char *text = "echo msg=\"Hello world\" rep=100";
    StrSlice ss = { .items = text, .len = strlen(text) };

    Lexer lexer = ksh_lexer_new(ss);

    printf("%d\n", ksh_lexer_is_token_next(&lexer, TOKEN_TYPE_LIT));
    Token t = ksh_lexer_expect_next_token(&lexer, TOKEN_TYPE_LIT);
    strslice_print(t.text);
    putchar('\n');

    lexer.cursor = 0;

    Token token = ksh_lexer_next_token(&lexer);
    while (token.type != TOKEN_TYPE_END) {
        strslice_print(token.text);
        printf(" -> %s\n", ksh_lexer_token_type_to_string(token.type));

        token = ksh_lexer_next_token(&lexer);
    }

    putchar('\n');

    lexer.cursor = 0;

    Context context = ksh_context_new();
    ksh_context_init(&lexer, &context);
    ContextIter iter = { .context = context };

    printf("%zu\n", iter.context.entries_len);

    ContextEntry *entry;
    while (ksh_context_iter_next(&iter, &entry)) {
        strslice_print(entry->token.text);
        printf(" -> %s\n", ksh_context_use_to_str(entry->ctx_use));
    }
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

static bool ksh_lexer_match_pattern(Lexer *l, TTPattern pat)
{
    size_t checkpoint = l->cursor;
    for (size_t i = 0; i < pat.len; i++) {
        if (ksh_lexer_is_token_next(l, pat.items[i].tt)) {
            ksh_lexer_inc(l, (ksh_lexer_make_token(l, pat.items[i].tt)).text.len);
        } else {
            l->cursor = checkpoint;
            return false;
        }
    }

    l->cursor = checkpoint;
    return true;
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
