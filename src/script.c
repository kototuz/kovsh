#include "kovsh.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

// AVAILABLE TOKEN SEQUENCES
#define TOKEN_TYPE_SEQS_LEN (sizeof(token_type_seqs) / sizeof(token_type_seqs[0]))
static const struct TokenTypeSeq {
    size_t len;
    TokenType *items;
    ContextUse ctx_use;
} token_type_seqs[] = {
    {
        .items = (TokenType[]){ TOKEN_TYPE_LIT, TOKEN_TYPE_EQ, TOKEN_TYPE_STRING },
        .len = 3,
        .ctx_use = CONTEXT_USE_ARG_ASSIGN
    },
    {
        .items = (TokenType[]){ TOKEN_TYPE_LIT, TOKEN_TYPE_EQ, TOKEN_TYPE_NUMBER },
        .len = 3,
        .ctx_use = CONTEXT_USE_ARG_ASSIGN
    },
    {
        .items = (TokenType[]){ TOKEN_TYPE_LIT, TOKEN_TYPE_EQ, TOKEN_TYPE_BOOL },
        .len = 3,
        .ctx_use = CONTEXT_USE_ARG_ASSIGN
    },
    {
        .items = (TokenType[]){ TOKEN_TYPE_LIT },
        .len = 1,
        .ctx_use = CONTEXT_USE_CMD_CALL
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
static bool ksh_token_seq_is_next(Lexer *l, struct TokenTypeSeq tts);


// PUBLIC FUNCTIONS
Lexer ksh_lexer_new(StrSlice ss)
{
    return (Lexer) { .text = ss };
}

TokenType ksh_lexer_compute_token_type(Lexer *l)
{
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
    lexer_trim(l);
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

// Token ksh_lexer_next_token(Lexer *l)
// {
//     Token result = {0};
// 
//     lexer_trim(l);
//     size_t cursor = l->cursor;
// 
//     int first_letter = l->text.items[cursor];
//     switch (first_letter) {
//         case '=':
//             result.type = TOKEN_TYPE_EQ;
//             result.text.items = &l->text.items[cursor++];
//             result.text.len = 1;
//             break;
// 
//         case '"':
//             result.type = TOKEN_TYPE_STRING;
//             result.text.items = &l->text.items[++cursor];
//             while (l->text.items[cursor] != '"') {
//                 if (l->text.items[cursor] == '\0') {
//                     fputs("ERROR: the closing quote was missed\n", stderr);
//                     exit(1);
//                 }
// 
//                 result.text.len++;
//                 cursor++;
//             }
//             cursor++;
//             break;
// 
//         case '\0':
//             result.type = TOKEN_TYPE_END;
//             break;
// 
//         default:
//             l->cursor = cursor;
//             if (lexer_at_keyword(l, BOOL_FALSE_KEYWORD)) {
//                 result.type = TOKEN_TYPE_BOOL;
//                 result.text = BOOL_FALSE_KEYWORD;
// 
//                 cursor += BOOL_FALSE_KEYWORD.len;
//             } else if (lexer_at_keyword(l, BOOL_TRUE_KEYWORD)) {
//                 result.type = TOKEN_TYPE_BOOL;
//                 result.text = BOOL_TRUE_KEYWORD;
// 
//                 cursor += BOOL_TRUE_KEYWORD.len;
//             } else if (isdigit(l->text.items[cursor])) {
//                 result.type = TOKEN_TYPE_NUMBER;
//                 result.text.items = &l->text.items[cursor++];
//                 result.text.len = 1;
//                 while (isdigit(l->text.items[cursor])) {
//                     cursor++;
//                     result.text.len++;
//                 }
//             } else if (is_lit(l->text.items[cursor])) {
//                 result.type = TOKEN_TYPE_LIT;
//                 result.text.items = &l->text.items[cursor++];
//                 result.text.len = 1;
//                 while (is_lit(l->text.items[cursor])) {
//                     cursor++;
//                     result.text.len++;
//                 }
//             }
// 
//             break;
//     }
// 
//     l->cursor = cursor;
// 
//     return result;
// }

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

ContextEntry *ksh_context_entry_create(TokenSeq seq, ContextUse ctx_use) {
    ContextEntry *result = (ContextEntry *)malloc(sizeof(ContextEntry));
    if (!result) {
        fputs("ERROR: could not allocate memmory for `ContextEntry`\n", stderr);
        exit(1);
    }

    *result = (ContextEntry) {
        .ts = seq,
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

void ksh_lexer_parse_to_context(Lexer *l, Context *context)
{
    for (size_t i = 0; i < TOKEN_TYPE_SEQS_LEN;) {
        struct TokenTypeSeq seq = token_type_seqs[i];
        if (ksh_token_seq_is_next(l, seq)) {
            TokenSeq token_seq = ksh_token_seq_from_lexer(l, seq.len);
            ContextEntry *ctx_entry = ksh_context_entry_create(token_seq, seq.ctx_use);
            ksh_context_write(context, ctx_entry);
            i = 0;
            continue;
        }
        i++;
    }
}

const char *ksh_context_use_to_str(ContextUse ctx_use)
{
    switch (ctx_use) {
        case CONTEXT_USE_CMD_CALL: return "cmd call";
        case CONTEXT_USE_ARG_ASSIGN: return "arg assign";
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
    const char *text = "echo msg=\"Hello world\"";
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
    ksh_lexer_parse_to_context(&lexer, &context);
    ContextIter iter = { .context = context };

    printf("%zu\n", iter.context.entries_len);

    ContextEntry *entry;
    while (ksh_context_iter_next(&iter, &entry)) {
        for (size_t i = 0; i < entry->ts.len; i++) {
            strslice_print(entry->ts.items[i].text);
            printf(" -> %s\n" , ksh_context_use_to_str(entry->ctx_use));
        }
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

static bool ksh_token_seq_is_next(Lexer *l, struct TokenTypeSeq tts)
{
    bool result = true;
    size_t checkpoint = l->cursor;
    for (size_t i = 0; i < tts.len; i++) {
        Token token = ksh_lexer_next_token(l);
        if (token.type != tts.items[i]) {
            result = false;
            break;
        }
    }

    l->cursor = checkpoint;
    return result;
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
