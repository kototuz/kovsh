#ifndef KOVSH_CMD_H
#define KOVSH_CMD_H

#include <stddef.h>

typedef int (*CmdCbFn)(void *self);
typedef struct {
    void *ctx;
    CmdCbFn run;
} CmdCb;

#define CMD_CB(cb) (_Generic((cb), \
    char *: (CmdCb){(cb), (CmdCbFn)cmd_cb_printer_run}, \
    CmdCbWithArgs *: (CmdCb){(cb), (CmdCbFn)cmd_cb_with_args_run}, \
    default: (CmdCb){NULL, (CmdCbFn)NULL}))

typedef struct {
    const char *name;
    const char *desc;
    CmdCb callback;
} Cmd;

Cmd *cmd_init(Cmd snip);
int cmd_run(Cmd *self);

int   cmd_cb_printer_run(const char *msg);

typedef struct CmdArg CmdArg;
typedef void (*CmdArgParser)(CmdArg *arg, const char *ctx);

int cmd_strarg_parser(CmdArg *arg, const char *ctx);
int cmd_intarg_parser(CmdArg *arg, const char *ctx);

struct CmdArg {
    const char *name;
    CmdArgParser parser;
    union {
        const char *as_str;
        int as_int;
    };
};

#define CMD_ARG_LIST(args) \
    (CmdArgList){sizeof((args)) / sizeof(CmdArg), (args)}
typedef struct {
    size_t len;
    CmdArg *data;
} CmdArgList;

typedef struct CmdCbWithArgs {
    CmdArgList arg_list;
    void (*handler)(struct CmdCbWithArgs *self);
} CmdCbWithArgs;

int cmd_cb_with_args_run(CmdCbWithArgs *self);

#endif
