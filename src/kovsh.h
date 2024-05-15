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
    KSH_ERR_TYPE_EXPECTED,
    KSH_ERR_ASSIGNMENT_EXPECTED,
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
    ARG_VAL_TYPE_STR,
    ARG_VAL_TYPE_INT,
    ARG_VAL_TYPE_BOOL,
} ArgValType;

typedef union {
    StrView as_str;
    int as_int;
    bool as_bool;
} ArgVal; 

typedef struct {
    StrView name;
    const char *usage;
    bool has_default;
    ArgValType type;
    ArgVal default_val;
} ArgDef;

typedef struct {
    ArgDef *def;
    bool is_assign;
    ArgVal value;
} Arg;

typedef int (*CommandFn)(size_t argc, Arg argv[argc]);

typedef struct {
    StrView name;
    const char *desc;
    CommandFn fn;
    ArgDef *arg_defs;
    size_t arg_defs_len;
} Command;

typedef struct {
    Command *cmd;
    Arg *argv;
    size_t argc;
} CommandCall;

typedef struct {
    Command *items;
    size_t len;
} CommandBuf;

Command *ksh_cmds_add(Command cmd);

void ksh_cmd_print(Command cmd);
Command *ksh_cmd_find(StrView sv);
CommandCall ksh_cmd_create_call(Command *cmd);
ArgDef *ksh_cmd_find_arg_def(Command *cmd, StrView name);

KshErr ksh_cmd_call_exec(CommandCall call);

const char *ksh_arg_val_type_to_str(ArgValType avt);
void ksh_arg_def_print(ArgDef cmd_arg);

Arg *ksh_args_find(size_t argc, Arg argv[argc], StrView sv);

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

KshErr ksh_token_parse_to_arg(Token token, Arg *arg);
KshErr ksh_parse_lexer(Lexer *l);


///////////////////////////////////////////////
/// TERM
//////////////////////////////////////////////

typedef struct {
    CommandBuf cmd_buf;
    const char *prompt;
} Terminal;

void ksh_term_start(Terminal term);

#endif
