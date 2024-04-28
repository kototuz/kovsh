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

// KEYWORDS
const StrSlice BOOL_TRUE_KEYWORD = { .items = "true", .len = 4 };
const StrSlice BOOL_FALSE_KEYWORD = { .items = "false", .len = 5 };

// PRIVATE FUNCTION DECLARATIONS
static void lexer_trim(Lexer *l);
static bool is_lit(int letter);
static bool lexer_at_keyword(Lexer *l, StrSlice keyword);
static bool lexer_token_seq_next_is(Lexer *l, struct TokenTypeSeq tts);


// PUBLIC FUNCTIONS
Lexer lexer_new(StrSlice ss)
{
    return (Lexer) { .text = ss };
}

const char *lexer_token_type_to_string(TokenType token_type)
{
    switch (token_type) {
    case TOKEN_TYPE_LIT: return "literal";
    case TOKEN_TYPE_STRING: return "string";
    case TOKEN_TYPE_NUMBER: return "number";
    case TOKEN_TYPE_BOOL: return "bool";
    case TOKEN_TYPE_EQ: return "equal";
    case TOKEN_TYPE_END: return "end";
    default: return "invalid";
    }
}

Token lexer_token_next(Lexer *l)
{
    Token result = {0};

    lexer_trim(l);
    size_t cursor = l->cursor;

    int first_letter = l->text.items[cursor];
    switch (first_letter) {
        case '=':
            result.type = TOKEN_TYPE_EQ;
            result.text.items = &l->text.items[cursor++];
            result.text.len = 1;
            break;

        case '"':
            result.type = TOKEN_TYPE_STRING;
            result.text.items = &l->text.items[++cursor];
            while (l->text.items[cursor] != '"') {
                if (l->text.items[cursor] == '\0') {
                    fputs("ERROR: the closing quote was missed\n", stderr);
                    exit(1);
                }

                result.text.len++;
                cursor++;
            }
            cursor++;
            break;

        case '\0':
            result.type = TOKEN_TYPE_END;
            break;

        default:
            l->cursor = cursor;
            if (lexer_at_keyword(l, BOOL_FALSE_KEYWORD)) {
                result.type = TOKEN_TYPE_BOOL;
                result.text = BOOL_FALSE_KEYWORD;

                cursor += BOOL_FALSE_KEYWORD.len;
            } else if (lexer_at_keyword(l, BOOL_TRUE_KEYWORD)) {
                result.type = TOKEN_TYPE_BOOL;
                result.text = BOOL_TRUE_KEYWORD;

                cursor += BOOL_TRUE_KEYWORD.len;
            } else if (isdigit(l->text.items[cursor])) {
                result.type = TOKEN_TYPE_NUMBER;
                result.text.items = &l->text.items[cursor++];
                result.text.len = 1;
                while (isdigit(l->text.items[cursor])) {
                    cursor++;
                    result.text.len++;
                }
            } else if (is_lit(l->text.items[cursor])) {
                result.type = TOKEN_TYPE_LIT;
                result.text.items = &l->text.items[cursor++];
                result.text.len = 1;
                while (is_lit(l->text.items[cursor])) {
                    cursor++;
                    result.text.len++;
                }
            }

            break;
    }

    l->cursor = cursor;

    return result;
}

void lexer_inc(Lexer *l, size_t inc)
{
    l->cur_letter = l->text.items[(l->cursor += inc)];
}

Token lexer_token_next_expect(Lexer *l, TokenType expect)
{
    Token t = lexer_token_next(l);
    if (t.type != expect) {
        fprintf(stderr, "ERROR: %s was expected, but %s is occured\n",
               lexer_token_type_to_string(expect),
               lexer_token_type_to_string(t.type));
        exit(1);
    }

    return t;
}

bool lexer_token_next_is(Lexer *l, TokenType t)
{
    size_t checkpoint = l->cursor;
    Token token = lexer_token_next(l);
    l->cursor = checkpoint;
    return token.type == t;
}

bool lexer_token_seq_next_is(Lexer *l, struct TokenTypeSeq tts)
{
    bool result = true;;
    size_t checkpoint = l->cursor;
    for (size_t i = 0; i < tts.len; i++) {
        Token token = lexer_token_next(l);
        if (token.type != tts.items[i]) {
            result = false;
            break;
        }
    }

    l->cursor = checkpoint;
    return result;
}

TokenSeq lexer_token_seq_from_lexer(Lexer *l, size_t count)
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
        Token token = lexer_token_next(l);
        if (token.type == TOKEN_TYPE_END) {
            fprintf(stderr, "ERROR: could not get %zu tokens: tokens are over", count);
            exit(1);
        }
        result.items[i] = token;
    }

    return result;
}

void lexer_context_delete(Context *context)
{
    ContextEntry *tmp;
    ContextEntry *it = context->first;
    while (it != NULL) {
        tmp = it;
        it = it->next;
        free(tmp);
    }
}

Context lexer_context_new(void)
{
    return (Context) { .first = NULL, .last = NULL };
}

ContextEntry *lexer_context_entry_create(TokenSeq seq, ContextUse ctx_use) {
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

void lexer_context_write(Context *context, ContextEntry *ce)
{
    if (context->first == NULL) {
        context->first = context->last = ce;
    } else {
        context->last = context->last->next = ce;
    }
    context->entries_len++;
}

void lexer_parse_to_context(Lexer *l, Context *context)
{
    for (size_t i = 0; i < TOKEN_TYPE_SEQS_LEN;) {
        struct TokenTypeSeq seq = token_type_seqs[i];
        if (lexer_token_seq_next_is(l, seq)) {
            TokenSeq token_seq = lexer_token_seq_from_lexer(l, seq.len);
            ContextEntry *ctx_entry = lexer_context_entry_create(token_seq, seq.ctx_use);
            lexer_context_write(context, ctx_entry);
            i = 0;
            continue;
        }
        i++;
    }
}

const char *lexer_context_use_to_str(ContextUse ctx_use)
{
    switch (ctx_use) {
        case CONTEXT_USE_CMD_CALL: return "cmd call";
        case CONTEXT_USE_ARG_ASSIGN: return "arg assign";
        default: return "unknown";
    }
}

bool lexer_context_iter_next(ContextIter *ctx_iter, ContextEntry **output)
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
    const char *text = "echo msg=\"Hello world\" rep=123";
    StrSlice ss = { .items = text, .len = strlen(text) };

    Lexer lexer = lexer_new(ss);

    Token token = lexer_token_next(&lexer);
    while (token.type != TOKEN_TYPE_END) {
        strslice_print(token.text);
        printf(" -> %s\n", lexer_token_type_to_string(token.type));

        token = lexer_token_next(&lexer);
    }

    putchar('\n');

    lexer.cursor = 0;

    Context context = lexer_context_new();
    lexer_parse_to_context(&lexer, &context);
    ContextIter iter = { .context = context };

    printf("%zu\n", iter.context.entries_len);

    ContextEntry *entry;
    while (lexer_context_iter_next(&iter, &entry)) {
        for (size_t i = 0; i < entry->ts.len; i++) {
            strslice_print(entry->ts.items[i].text);
            printf(" -> %s\n" ,lexer_context_use_to_str(entry->ctx_use));
        }
    }
}

// PRIVATE FUNCTIONS
static void lexer_trim(Lexer *self) {
    while (self->text.items[self->cursor] == ' ') {
        self->cursor++;
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
