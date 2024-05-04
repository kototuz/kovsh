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
} Lexer;

typedef enum {
    TOKEN_TYPE_LIT,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_EQ,
    TOKEN_TYPE_INVALID,

    TOKEN_TYPE_END,
    TOKEN_TYPE_ENUM_END
} TokenType;

typedef struct {
    TokenType type;
    StrSlice text;
} Token;

Lexer ksh_lexer_new(StrSlice ss);

const char *ksh_lexer_token_type_to_string(TokenType token_type);
void ksh_lexer_inc(Lexer *l, size_t inc);

TokenType ksh_lexer_compute_token_type(Lexer *l);
Token ksh_lexer_make_token(Lexer *l, TokenType tt);
Token ksh_lexer_expect_next_token(Lexer *l, TokenType expect);
Token ksh_lexer_next_token(Lexer *l);
bool  ksh_lexer_is_token_next(Lexer *l, TokenType t);

#endif
