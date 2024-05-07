#ifndef KOVSH_H
#define KOVSH_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

// UTILS
typedef struct {
    size_t len;
    const char *items;
} StrSlice;

StrSlice strslice_from_bounds(char *begin, char *end);
void strslice_new(StrSlice *ss, size_t len, char items[*]);
void strslice_print(StrSlice s, FILE *output);
int  strslice_cmp(StrSlice ss1, StrSlice ss2);

///////////////////////////////////////////////
/// COMMAND
//////////////////////////////////////////////


typedef enum {
    COMMAND_ARG_VAL_KIND_STR,
    COMMAND_ARG_VAL_KIND_INT,
    COMMAND_ARG_VAL_KIND_BOOL,
} CommandArgValKind;

typedef struct {
    CommandArgValKind kind;
    union {
        StrSlice as_str;
        int as_int;
        bool as_bool;
    };
} CommandArgVal;

typedef struct {
    StrSlice name;
    const char *usage;
    CommandArgVal value;
} CommandArg;

typedef int (*CommandFn)(size_t argc, CommandArg argv[argc]);

typedef struct {
    StrSlice name;
    const char *desc;
    CommandFn fn;
    CommandArg *expected_args;
    size_t expected_args_len;
} Command;

typedef struct {
    CommandFn fn;
    CommandArg *argv;
    size_t argc;
} CommandCall;

Command *ksh_commands_add(Command cmd);

void ksh_command_print(Command cmd);
Command *ksh_command_find(StrSlice ss);
CommandCall ksh_command_create_call(Command *cmd);

void ksh_commandcall_set_arg(CommandCall *cmd_call, StrSlice arg_name, CommandArgVal value);
void ksh_commandcall_exec(CommandCall call);

const char *ksh_commandargvalkind_to_str(CommandArgValKind cavt);
void ksh_commandarg_print(CommandArg cmd_arg);
CommandArg *ksh_commandarg_find(size_t len, CommandArg args[len], StrSlice ss);


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

CommandArgVal ksh_token_parse_to_arg_val(Token token);
void ksh_parse_lexer(Lexer *l);

#endif
