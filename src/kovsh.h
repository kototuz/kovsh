#ifndef KOVSH_H
#define KOVSH_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>

///////////////////////////////////////////////
/// COMMON
//////////////////////////////////////////////

#define KSH_LOG_ERR(msg, ...) printf("[KOVSH ERR]: "msg, __VA_ARGS__)

#define STRV_LIT(lit) { sizeof(lit)-1, (lit) }
#define STRV_FMT "%.*s"
#define STRV_ARG(sv) (int) (sv).len, (sv).items

typedef enum {
    KSH_ERR_OK = 0,
    KSH_ERR_COMMAND_NOT_FOUND,
    KSH_ERR_ARG_NOT_FOUND,
    KSH_ERR_TOKEN_EXPECTED,
    KSH_ERR_TYPE_EXPECTED,
    KSH_ERR_ASSIGNMENT_EXPECTED,
    KSH_ERR_MEM_OVER,
    KSH_ERR_PATTERN_NOT_FOUND,
    KSH_ERR_VAR_NOT_FOUND
} KshErr;

typedef struct {
    size_t len;
    const char *items;
} StrView;

typedef enum {
    KSH_VALUE_TYPE_TAG_STR,
    KSH_VALUE_TYPE_TAG_INT,
    KSH_VALUE_TYPE_TAG_BOOL,
    KSH_VALUE_TYPE_TAG_ANY,
    KSH_VALUE_TYPE_TAG_ENUM
} KshValueTypeTag;

typedef struct {
    void *info;
    KshValueTypeTag tag;
} KshValueType;

typedef struct {
    StrView *cases;
    size_t cases_len;
} KshValueTypeEnum;

typedef union {
    StrView as_str;
    int     as_int;
    bool    as_bool;
} KshValue;

const char *ksh_err_str(KshErr err);

StrView strv_from_str(const char *str);
StrView strv_new(const char *data, size_t data_len);
bool    strv_eq(StrView sv1, StrView sv2);

const char *ksh_val_type_tag_str(KshValueTypeTag t);
KshErr      ksh_val_parse(KshValueType type, KshValue *value);

///////////////////////////////////////////////
/// COMMAND
//////////////////////////////////////////////

typedef struct {
    KshValue data;
    bool is_assigned;
} ArgValue;

typedef struct {
    StrView name;
    const char *usage;
    KshValueType value_type;
    ArgValue value;
} Arg;

typedef struct {
    Arg *arg_ptr;
    ArgValue value;
} ArgValueCopy;

typedef int (*CommandFn)(ArgValueCopy *args);

typedef struct {
    CommandFn fn;
    ArgValueCopy *args;
    size_t args_len;
    size_t last_assigned_idx;
} CommandCall;

typedef struct Command Command;

typedef struct {
    Command *items;
    size_t len;
} CommandSet;

struct Command {
    CommandFn fn;
    StrView name;
    const char *desc;
    Arg *args;
    size_t args_len;
    CommandSet subcommands;
};


void ksh_cmd_print(Command cmd);
Command *ksh_cmd_find_local(CommandSet set, StrView sv);
Command *ksh_cmd_find_hardcoded(StrView sv);
Command *ksh_cmd_find(CommandSet set, StrView sv);
KshErr ksh_cmd_init_call(Command *cmd, CommandCall *call);
KshErr ksh_cmd_get(CommandSet set, StrView sv, Command *out);

KshErr ksh_cmd_call_execute(CommandCall);
ArgValueCopy *ksh_cmd_call_find_value(CommandCall *call, StrView name);

void ksh_arg_print(Arg);

///////////////////////////////////////////////
/// LEXER
//////////////////////////////////////////////

typedef enum {
    TOKEN_TYPE_LIT,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_VAR,
    TOKEN_TYPE_EQ,
    TOKEN_TYPE_KEYWORD_SYS,
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

Lexer ksh_lexer_new(StrView sv);
const char *ksh_lexer_token_type_to_string(TokenType token_type);
bool ksh_lexer_peek_token(Lexer *l, Token *t);
bool ksh_lexer_next_token(Lexer *l, Token *t);
bool ksh_lexer_is_next_token(Lexer *l, TokenType tt);
bool ksh_lexer_next_token_if(Lexer *l, TokenType tt, Token *t);
KshErr ksh_lexer_expect_next_token(Lexer *l, TokenType expect, Token *out);

bool   ksh_token_type_fit_value_type(TokenType, KshValueTypeTag);
KshErr ksh_token_parse_to_value(Token tok, KshValueType type, KshValue *dest);
KshErr ksh_token_from_strv(StrView sv, Token *dest);
KshErr ksh_token_type_to_value(TokenType t, KshValueTypeTag *dest);

///////////////////////////////////////////////
/// MAIN USAGE
//////////////////////////////////////////////

typedef struct {
    StrView name;
    struct {
        char  *items;
        size_t len;
    } value;
} Variable;

typedef struct {
    Variable *items;
    size_t len;
} VariableSet;

void ksh_init(void);
void ksh_deinit(void);

KshErr ksh_parse(StrView, CommandCall *dest);

void ksh_use_command_set(CommandSet);
void ksh_use_variable_set(VariableSet);

KshErr ksh_var_add(const StrView name, const StrView value);
KshErr ksh_var_get_val(StrView name, StrView *dest);

#endif
