#ifndef KOVSH_H
#define KOVSH_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>

#define STRV_LIT(lit) { sizeof(lit)-1, (lit) }
#define STRV_LIT_DYN(lit) (StrView){ sizeof(lit)-1, (lit) }
#define STRV_FMT "%.*s"
#define STRV_ARG(sv) (int) (sv).len, (sv).items

typedef enum {
    KSH_ERR_OK = 0,
    KSH_ERR_COMMAND_NOT_FOUND,
    KSH_ERR_ARG_NOT_FOUND,
    KSH_ERR_TOKEN_EXPECTED,
    KSH_ERR_UNDEFINED_TOKEN,
    KSH_ERR_TYPE_EXPECTED,
    KSH_ERR_ASSIGNMENT_EXPECTED,
    KSH_ERR_MEM_OVER,
    KSH_ERR_VAR_NOT_FOUND,
    KSH_ERR_NAME_ALREADY_EXISTS,
    KSH_ERR_CONTEXT,
} KshErr;

typedef struct {
    size_t len;
    const char *items;
} StrView;

typedef StrView Token;

StrView strv_from_str(const char *str);
StrView strv_new(const char *data, size_t data_len);
bool    strv_eq(StrView sv1, StrView sv2);

typedef struct {
    StrView name;
    StrView value;
} KshArg;

typedef struct {
    KshArg *args;
    size_t args_count;
} KshContext;

typedef int (*KshCommandFn)(KshContext);

typedef struct {
    StrView name;
    KshCommandFn fn;
    size_t max_args;
    const char *description;
} KshCommand;

typedef struct {
    KshCommand *items;
    size_t count;
} KshCommands;

typedef enum {
    KSH_PARAM_TYPE_STR,
    KSH_PARAM_TYPE_INT,
} KshParamType;

void ksh_use_commands(KshCommands commands);

bool ksh_ctx_get_param(KshContext ctx, StrView name, KshParamType param_type, void *res);
bool ksh_ctx_get_option(KshContext ctx, StrView name);

#endif
