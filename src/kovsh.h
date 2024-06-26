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

typedef struct {
    StrView name;
    StrView value;
} KshArg;

typedef struct {
    StrView text;
    size_t cursor;
    Token buf;
} Lexer;

typedef struct {
    Lexer *lex;
    KshArg *args;
    size_t args_count;
} KshContext;

typedef int (*KshCommandFn)(KshContext);

typedef struct {
    StrView name;
    KshCommandFn fn;
} KshCommand;

typedef struct {
    KshCommand *items;
    size_t count;
} KshCommands;

typedef enum {
    KSH_PARAM_TYPE_STR,
    KSH_PARAM_TYPE_INT,
} KshParamType;

#define ksh_use_commands(...) \
    ksh_use_commands_( \
        sizeof((KshCommand[]){__VA_ARGS__})/sizeof(KshCommand), \
        (KshCommand[]){__VA_ARGS__} \
    ) \

void ksh_use_commands_(size_t size, KshCommand buf[size]);

#define ksh_ctx_init(ctx, count) ksh_ctx_init_((ctx), (count), (KshArg[(count)]){0})

KshErr ksh_ctx_init_(KshContext *ctx, size_t size, KshArg arg_buf[size]);
bool ksh_ctx_get_param(KshContext *ctx, StrView name, KshParamType param_type, void *res);
bool ksh_ctx_get_option(KshContext *ctx, StrView name);

#endif
