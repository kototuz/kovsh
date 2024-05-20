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
    KSH_ERR_MEM_OVER,
    KSH_ERR_PATTERN_NOT_FOUND
} KshErr;

const char *ksh_err_str(KshErr err);

typedef struct {
    size_t len;
    const char *items;
} StrView;
#define STRV_LIT(lit) { sizeof(lit)-1, (lit) }

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
    size_t last_assigned;
} CommandCall;

typedef struct {
    Command *items;
    size_t len;
} CommandBuf;

void ksh_cmd_print(Command cmd);
Command *ksh_cmd_find(CommandBuf buf, StrView sv);
CommandCall ksh_cmd_create_call(Command *cmd);
ArgDef *ksh_cmd_find_arg_def(Command *cmd, StrView name);

KshErr ksh_cmd_call_exec(CommandCall call);

const char *ksh_arg_val_type_to_str(ArgValType avt);
void ksh_arg_def_print(ArgDef cmd_arg);

Arg *ksh_args_find(size_t argc, Arg argv[argc], StrView sv);

///////////////////////////////////////////////
/// LEXER
//////////////////////////////////////////////


typedef enum {
    TOKEN_TYPE_LIT,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_EQ,
    TOKEN_TYPE_INVALID,

    TOKEN_TYPE_END,
    TOKEN_TYPE_ENUM_END,
    TOKEN_TYPE_PLUS
} TokenType;

typedef struct {
    TokenType type;
    StrView text;
} Token;

typedef struct {
    StrView text;
    size_t cursor;
    Token buf;
} Lexer;

typedef struct {
    TokenType *seq;
    size_t seq_len;
    KshErr (*eval_fn)(Token *toks, CommandCall *cmd_call);
} TokenPattern;

typedef struct {
    Token *items;
    size_t len;
} TokenSeq;

typedef struct {
    CommandBuf cmds;
} ParseInfo;

typedef struct {
    ParseInfo info;
    Lexer lex;
} Parser;

Lexer ksh_lexer_new(StrView sv);
const char *ksh_lexer_token_type_to_string(TokenType token_type);
bool ksh_lexer_peek_token(Lexer *l, Token *t);
bool ksh_lexer_next_token(Lexer *l, Token *t);
bool ksh_lexer_is_next_token(Lexer *l, TokenType tt);
KshErr ksh_lexer_expect_next_token(Lexer *l, TokenType expect, Token *out);

ArgVal ksh_token_to_arg_val(Token tok);

KshErr ksh_parse(Parser *p);

///////////////////////////////////////////////
/// TERM
//////////////////////////////////////////////

typedef struct {
    CommandBuf cmd_buf;
    const char *prompt;
} Terminal;

void ksh_term_start(Terminal term);

#endif
