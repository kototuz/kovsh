#ifndef KOVSH_H
#define KOVSH_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#define KSH_LOG_ERR(msg, ...) fprintf(stderr, "[KOVSH ERR]: "msg"\n", __VA_ARGS__)

// UTILS
// ERRORS
typedef enum {
    KSH_ERR_OK = 0,
    KSH_ERR_COMMAND_NOT_FOUND,
    KSH_ERR_ARG_NOT_FOUND,
    KSH_ERR_TOKEN_EXPECTED,
    KSH_ERR_TYPE_EXPECTED
} KshErr;

const char *ksh_err_str(KshErr err);

typedef struct {
    size_t len;
    const char *items;
} StrView;

#define STRV_LIT(lit) (strv_new(lit, sizeof(lit)-1))

#define STRV_FMT "%.*s"
#define STRV_ARG(sv) (int) (sv).len, (sv).items

StrView strv_new(const char *data, size_t data_len);
bool strv_eq(StrView sv1, StrView sv2);

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
        StrView as_str;
        int as_int;
        bool as_bool;
    };
} CommandArgVal;

typedef struct {
    StrView name;
    const char *usage;
    CommandArgVal value;
} CommandArg;

typedef int (*CommandFn)(size_t argc, CommandArg argv[argc]);

typedef struct {
    StrView name;
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
Command *ksh_command_find(StrView sv);
CommandCall ksh_command_create_call(Command *cmd);

KshErr ksh_commandcall_set_arg(CommandCall *cmd_call, StrView arg_name, CommandArgVal value);
void ksh_commandcall_exec(CommandCall call);

const char *ksh_commandargvalkind_to_str(CommandArgValKind cavt);
void ksh_commandarg_print(CommandArg cmd_arg);
CommandArg *ksh_commandarg_find(size_t len, CommandArg args[len], StrView sv);


///////////////////////////////////////////////
/// LEXER
//////////////////////////////////////////////

typedef struct {
    StrView text;
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
    StrView text;
} Token;

Lexer ksh_lexer_new(StrView sv);

const char *ksh_lexer_token_type_to_string(TokenType token_type);
void ksh_lexer_inc(Lexer *l, size_t inc);

TokenType ksh_lexer_compute_token_type(Lexer *l);
Token ksh_lexer_make_token(Lexer *l, TokenType tt);
KshErr ksh_lexer_expect_next_token(Lexer *l, TokenType expect, Token *out);
Token ksh_lexer_next_token(Lexer *l);
bool  ksh_lexer_is_token_next(Lexer *l, TokenType t);

CommandArgVal ksh_token_parse_to_arg_val(Token token);
KshErr ksh_parse_lexer(Lexer *l);

#endif
