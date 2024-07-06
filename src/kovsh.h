#ifndef KOVSH_H
#define KOVSH_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>

#define STRV_LIT(lit) (StrView){ sizeof(lit)-1, (lit) }
#define STRV_FMT "%.*s"
#define STRV_ARG(sv) (int) (sv).len, (sv).items

typedef enum {
    KSH_ERR_OK = 0,
    KSH_ERR_COMMAND_NOT_FOUND,
    KSH_ERR_TOKEN_EXPECTED,
    KSH_ERR_ARG_NOT_FOUND,
    KSH_ERR_VALUE_EXPECTED,
    KSH_ERR_PARSING_FAILED,
    KSH_ERR_EARLY_EXIT,
} KshErr;

const char *ksh_err_str(KshErr err);

typedef struct {
    size_t len;
    const char *items;
} StrView;

typedef StrView Token;

StrView strv_from_str(const char *str);
StrView strv_new(const char *data, size_t data_len);
bool    strv_eq(StrView sv1, StrView sv2);

typedef enum {
    KSH_ARG_KIND_OPT,
    KSH_ARG_KIND_PARAM_STR,
    KSH_ARG_KIND_PARAM_INT,
    KSH_ARG_KIND_HELP,
    KSH_ARG_KIND_SUBCMD
} KshArgKind;
#define IS_PARAM(kind) (kind > KSH_ARG_KIND_OPT && kind <= KSH_ARG_KIND_PARAM_INT)

typedef struct {
    StrView text;
    size_t cursor;
    Token cur_tok;
    bool is_peek;
} Lexer;

typedef int (*KshCommandFn)(Lexer *l);

typedef struct {
    StrView name;
    const char *usage;
    KshArgKind kind;
    union {
        void *as_ptr;
        KshCommandFn as_fn;
    } data;
} KshArgDef;

typedef struct {
    KshArgDef *items;
    size_t count;
} KshArgDefs;

typedef struct {
    Lexer lex;
    KshArgDefs arg_defs;
} KshContext;


typedef struct {
    StrView name;
    KshCommandFn fn;
} KshCommand;

typedef struct {
    KshCommand *items;
    size_t count;
} KshCommands;

#define ksh_use_commands(...) \
    ksh_use_commands_( \
        sizeof((KshCommand[]){__VA_ARGS__})/sizeof(KshCommand), \
        (KshCommand[]){__VA_ARGS__} \
    ) \

void ksh_use_commands_(size_t size, KshCommand buf[size]);

#define KSH_OPT(var, usage) (KshArgDef){ STRV_LIT(#var), (usage), KSH_ARG_KIND_OPT, .data.as_ptr = &(var) }

#define KSH_PARAM(var, usage) (KshArgDef){ STRV_LIT(#var), (usage), _Generic((var), \
    int: KSH_ARG_KIND_PARAM_INT,                                                    \
    StrView: KSH_ARG_KIND_PARAM_STR,                                                \
    default: KSH_ARG_KIND_PARAM_STR), .data.as_ptr = &(var) }                       \

#define KSH_HELP(help) (KshArgDef){ STRV_LIT("help"), "prints this help", KSH_ARG_KIND_HELP, .data.as_ptr = (help) }

#define KSH_SUBCMD(fn, usage) (KshArgDef){ STRV_LIT(#fn), (usage), KSH_ARG_KIND_SUBCMD, .data.as_fn = (fn) }

#define ksh_parse_args(lex, ...) \
    ksh_parse_args_((lex), (KshArgDefs){ (KshArgDef[]){__VA_ARGS__}, sizeof((KshArgDef[]){__VA_ARGS__})/sizeof(KshArgDef) })

KshErr ksh_parse_args_(Lexer *l, KshArgDefs arg_defs);

#endif
