#ifndef KOVSH_H
#define KOVSH_H

#include <stddef.h>
#include <stdbool.h>

// UTILS
typedef struct {
    size_t len;
    const char *items;
} StrSlice;

StrSlice strslice_from_bounds(char *begin, char *end);
void strslice_new(StrSlice *ss, size_t len, char items[*]);
void strslice_print(StrSlice s);

///////////////////////////////////////////////
/// LEXER
//////////////////////////////////////////////

typedef struct {
    StrSlice text;
    size_t cursor;
    int cur_letter;
} Lexer;

typedef enum {
    TOKEN_TYPE_LIT,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_EQ,

    TOKEN_TYPE_END,
    TOKEN_TYPE_ENUM_END
} TokenType;

typedef struct {
    TokenType type;
    StrSlice text;
} Token;

typedef struct {
    size_t len;
    Token *items;
} TokenSeq;

typedef enum {
    CONTEXT_USE_CMD_CALL,
    CONTEXT_USE_ARG_ASSIGN,
} ContextUse;

typedef struct ContextEntry {
    TokenSeq ts;
    ContextUse ctx_use;
    struct ContextEntry *next;
} ContextEntry;

typedef struct {
    size_t entries_len;
    ContextEntry *first;
    ContextEntry *last;
} Context;

typedef struct {
    Context context;
    ContextEntry *cursor;
} ContextIter;

Lexer ksh_lexer_new(StrSlice ss);

const char *ksh_lexer_token_type_to_string(TokenType token_type);
void ksh_lexer_inc(Lexer *l, size_t inc);

Token ksh_lexer_expect_token_next(Lexer *l, TokenType expect);
Token ksh_lexer_token_next(Lexer *l);
bool  ksh_lexer_is_token_next(Lexer *l, TokenType t);

TokenSeq ksh_token_seq_from_lexer(Lexer *l, size_t count);

void ksh_lexer_parse_to_context(Lexer *l, Context *context);
void ksh_context_write(Context *context, ContextEntry *ce);
void ksh_context_delete(Context *context);
Context ksh_context_new(void);

ContextEntry *ksh_context_entry_create(TokenSeq ts, ContextUse ctx_use);
const char *ksh_context_use_to_str(ContextUse ctx_use);

bool ksh_context_iter_next(ContextIter *ctx_iter, ContextEntry **output);

#endif
