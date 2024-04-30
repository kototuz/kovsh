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

typedef Token (*LazyEvalFn)(Lexer *l);

// EVALUATE FUNCTION DECLARATIONS
static Token token_type_eq_eval(Lexer *l);
static Token token_type_lit_eval(Lexer *l);
static Token token_type_bool_eval(Lexer *l);
static Token token_type_string_eval(Lexer *l);
static Token token_type_number_eval(Lexer *l);
static Token token_type_empty(Lexer *l);

static const LazyEvalFn evaluate_functions[] = {
    [TOKEN_TYPE_EQ] = token_type_eq_eval,
    [TOKEN_TYPE_LIT] = token_type_lit_eval,
    [TOKEN_TYPE_BOOL] = token_type_bool_eval,
    [TOKEN_TYPE_STRING] = token_type_string_eval,
    [TOKEN_TYPE_NUMBER] = token_type_number_eval,
    [TOKEN_TYPE_END] = token_type_empty
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

LazyToken ksh_lexer_lazy_token_get(Lexer *l)
{
    lexer_trim(l);

    LazyToken result = {0};

    const int letter = l->text.items[l->cursor];
    switch (letter) {
    case '=':
        result.type = TOKEN_TYPE_EQ;
        break;
    case '"':
        result.type = TOKEN_TYPE_STRING;
        break;
    case '\0':
        result.type = TOKEN_TYPE_END;
        break;
    default:
        if (is_lit(letter)) {
            result.type = TOKEN_TYPE_LIT;
        } else if (isdigit(letter)) {
            result.type = TOKEN_TYPE_NUMBER;
        } else if (lexer_at_keyword(l, BOOL_TRUE_KEYWORD)
                   || lexer_at_keyword(l, BOOL_FALSE_KEYWORD)) {
            result.type = TOKEN_TYPE_BOOL;
        } else {
            assert(0 && "not yet implemented");
        }
        break;
    }
    result.l = l;

    return result;
}

Token ksh_lazy_token_evaluate(LazyToken lt)
{
    assert(lt.type < TOKEN_TYPE_ENUM_END);
    return evaluate_functions[lt.type](lt.l);
}

const char *ksh_lexer_token_type_to_string(TokenType token_type)
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

Token ksh_lexer_token_next(Lexer *l)
{
    Token token = ksh_lazy_token_evaluate(ksh_lexer_lazy_token_get(l));
    ksh_lexer_inc(l, token.text.len);
    return token;
}

// Token ksh_lexer_token_next(Lexer *l)
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

Token ksh_lexer_expect_token_next(Lexer *l, TokenType expect)
{
    LazyToken lt = ksh_lexer_lazy_token_get(l);
    if (lt.type != expect) {
        fprintf(stderr, "ERROR: %s was expected, but %s is occured\n",
                ksh_lexer_token_type_to_string(expect),
                ksh_lexer_token_type_to_string(lt.type));
        exit(1);
    }

    Token ret = ksh_lazy_token_evaluate(lt);
    ksh_lexer_inc(l, ret.text.len);
    return ret;
}

bool ksh_lexer_is_token_next(Lexer *l, TokenType t)
{
    LazyToken lt = ksh_lexer_lazy_token_get(l);
    return lt.type == t;
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
        Token token = ksh_lexer_token_next(l);
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

    printf("%d\n", ksh_lexer_is_token_next(&lexer, TOKEN_TYPE_STRING));

    Token token = ksh_lexer_token_next(&lexer);
    while (token.type != TOKEN_TYPE_END) {
        strslice_print(token.text);
        printf(" -> %s\n", ksh_lexer_token_type_to_string(token.type));

        token = ksh_lexer_token_next(&lexer);
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
    bool result = true;;
    size_t checkpoint = l->cursor;
    for (size_t i = 0; i < tts.len; i++) {
        Token token = ksh_lexer_token_next(l);
        if (token.type != tts.items[i]) {
            result = false;
            break;
        }
    }

    l->cursor = checkpoint;
    return result;
}

// EVALUATE FUNCTIONS
static Token token_type_eq_eval(Lexer *l)
{
    assert(l->text.items[l->cursor] == '=');
    return (Token){
        .text = (StrSlice){ .items = (char[]){ '=' }, .len = 1 },
        .type = TOKEN_TYPE_EQ
    };
}

static Token token_type_lit_eval(Lexer *l)
{
    size_t cursor = l->cursor;
    assert(is_lit(l->text.items[cursor]));
    Token result = {
        .text.items = &l->text.items[cursor++],
        .text.len = 1,
        .type = TOKEN_TYPE_LIT
    };

    while (is_lit(l->text.items[cursor])) {
        result.text.len++;
        cursor++;
    }

    return result;
}

static Token token_type_bool_eval(Lexer *l)
{
    size_t cursor = l->cursor;
    return (Token) {
        .text.items = &l->text.items[cursor],
        .text.len = l->text.items[cursor+4] == ' ' ? 4 : 5,
        .type = TOKEN_TYPE_BOOL
    };
}

static Token token_type_string_eval(Lexer *l)
{
    size_t cursor = l->cursor;
    assert(l->text.items[cursor] == '"');
    
    Token result = {
        .text.items = &l->text.items[cursor++],
        .text.len = 2,
        .type = TOKEN_TYPE_STRING
    };

    while (l->text.items[cursor] != '"') {
        if (l->text.items[cursor] == '\0') {
            fputs("ERROR: the closing quote was missed\n", stderr);
            exit(1);
        }

        result.text.len++;
        cursor++;
    }

    return result;
}

static Token token_type_number_eval(Lexer *l)
{
    size_t cursor = l->cursor;
    assert(isdigit(l->text.items[cursor]));
    Token result = {
        .text.items = &l->text.items[cursor],
        .text.len = 1,
        .type = TOKEN_TYPE_NUMBER
    };

    while (isdigit(l->text.items[cursor])) {
        result.text.len++;
        cursor++;
    }

    return result;
}

static Token token_type_empty(Lexer *l)
{
    (void)l;
    return (Token){ .type = TOKEN_TYPE_END };
}
