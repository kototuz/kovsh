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

#define MAX_ERR_MSG 100
typedef char KshErr[MAX_ERR_MSG];

typedef struct {
    size_t len;
    const char *items;
} StrView;

typedef StrView Token;

StrView strv_from_str(const char *str);
StrView strv_new(const char *data, size_t data_len);
bool    strv_eq(StrView sv1, StrView sv2);

typedef struct {
    StrView text;
    StrView buf;
    bool cargs;
} KshLexer;

typedef struct {
    StrView name;
    const char *usage;
} KshArg;

typedef enum {
    KSH_PARAM_TYPE_STR,
    KSH_PARAM_TYPE_INT,
    KSH_PARAM_TYPE_FLOAT,
} KshParamType;

typedef struct {
    KshArg base;
    KshParamType type;
    size_t max;
    void *var;
} KshParam;

typedef struct {
    KshParam *items;
    size_t count;
} KshParams;

typedef enum {
    KSH_FLAG_TYPE_STORE_BOOL,
    KSH_FLAG_TYPE_HELP,
} KshFlagType;

typedef struct {
    KshArg base;
    KshFlagType type;
    void *var;
} KshFlag;

typedef struct {
    KshFlag *items;
    size_t count;
} KshFlags;

struct KshParser;
typedef int (*KshCommandFn)(struct KshParser *p);
typedef struct {
    KshArg base;
    KshCommandFn fn;
} KshSubcmd;

typedef struct {
    KshSubcmd *items;
    size_t count;
} KshSubcmds;

typedef struct KshParser {
    KshLexer lex;
    KshParams params;
    KshFlags flags;
    KshSubcmds subcmds;
    KshErr err;
    KshCommandFn root;
} KshParser;

#define KSH_PARAMS(p, ...) (p)->params = (KshParams){ (KshParam[]){__VA_ARGS__}, sizeof((KshParam[]){__VA_ARGS__})/sizeof(KshParam) }
#define KSH_FLAGS(p, ...) (p)->flags = (KshFlags){ (KshFlag[]){__VA_ARGS__}, sizeof((KshFlag[]){__VA_ARGS__})/sizeof(KshFlag) }
#define KSH_SUBCMDS(p, ...) (p)->subcmds = (KshSubcmds){ (KshSubcmd[]){__VA_ARGS__}, sizeof((KshSubcmd[]){__VA_ARGS__})/sizeof(KshSubcmd) }

#define KSH_PARAM(PT, var, usage) (KshParam){ { STRV_LIT(#var), usage }, PT, sizeof(var)/(KSH_TYPESIZE(var)), &var }

#define KSH_SUBCMD(fn, descr) { { STRV_LIT(#fn), descr }, fn }

#define KSH_HELP(descr) { { STRV_LIT("help"), "displays this help" }, KSH_FLAG_TYPE_HELP, descr }

#define KSH_STORE(var, usage) _Generic(var,                                                  \
    StrView:  KSH_PARAM(KSH_PARAM_TYPE_STR, var, usage),                                     \
    StrView*: KSH_PARAM(KSH_PARAM_TYPE_STR, var, usage),                                     \
    int:      KSH_PARAM(KSH_PARAM_TYPE_INT, var, usage),                                     \
    int*:     KSH_PARAM(KSH_PARAM_TYPE_INT, var, usage),                                     \
    float:    KSH_PARAM(KSH_PARAM_TYPE_FLOAT, var, usage),                                   \
    float*:   KSH_PARAM(KSH_PARAM_TYPE_FLOAT, var, usage),                                   \
    bool:     (KshFlag){ { STRV_LIT(#var), usage }, KSH_FLAG_TYPE_STORE_BOOL, &var })        \

#define KSH_PARAM_TYPE(var) _Generic(var, \
    StrView:  KSH_PARAM_TYPE_STR,         \
    StrView*: KSH_PARAM_TYPE_STR,         \
    int:      KSH_PARAM_TYPE_INT,         \
    int*:     KSH_PARAM_TYPE_INT)         \

#define KSH_TYPESIZE(var) _Generic(var, \
    int*: sizeof(int),                  \
    StrView*: sizeof(StrView),          \
    default: sizeof(var))               \

bool ksh_parse(KshParser *p);
bool ksh_parse_cmd(KshParser *p, StrView cmd);
bool ksh_parse_cargs(KshParser *p, int argc, char **argv);

#endif
