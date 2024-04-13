#ifndef KOVSH_CMD_H
#define KOVSH_CMD_H

#include <stddef.h>

typedef struct CmdArg CmdArg;

#define DEFAULT_PARSE_INFO (CmdArgParseInfo){.beginSign = 0, .endSign = 0}
typedef struct {
    int beginSign;
    int endSign;
    int (*parser)(CmdArg *arg, const char *token);
} CmdArgParseInfo;

struct CmdArg {
    const char *name;
    CmdArgParseInfo parseInfo;
    union {
        const char *asStr;
        int asInt;
    } value;
};

#define cmdArgList(...) \
    (CmdArgList){ \
        .len = sizeof((CmdArg[]){__VA_ARGS__}) / sizeof(CmdArg), \
        .items = (CmdArg[]){__VA_ARGS__} \
    }
typedef struct {
    size_t len;
    CmdArg *items;
} CmdArgList;

typedef int (*CmdCbFn)(void *self, CmdArgList argList);
typedef struct {
    void *ctx;
    CmdCbFn run;
    CmdArgList argList;
} CmdCb;

typedef struct {
    const char *name;
    const char *desc;
    CmdCb callback;
} Cmd;

int cmdNoneCallback(void *self, CmdArgList list);
#define cmdSimple (Cmd){.desc = "none", .callback = (CmdCb){.ctx = NULL, .run = cmdNoneCallback}}

int cmdRun(Cmd *cmd);

int CMD_ARG_STR_PARSER(CmdArg *arg, const char *ctx);
int CMD_ARG_INT_PARSER(CmdArg *arg, const char *ctx);

#define cmdCbPrintCb(msg) (CmdCb) {.ctx = (msg), .run = (CmdCbFn)cmdCbPrintCbRun}
int cmdCbPrintCbRun(const char *msg, CmdArgList argList);

//////////////////////////////////////////////////////
///Cmd line
//////////////////////////////////////////////////////

#include <stdio.h>

#define CMDLINE_CAP 100


typedef struct {
    size_t len;
    Cmd *items;
} CmdList;

typedef struct {
    const char *reqMsg;
    size_t historyCap;
    CmdList cmdList;
} CmdlineConfig;

typedef struct {
    FILE *output;
    CmdlineConfig config;
} Cmdline;

Cmd *cmdlineRequestCmd(Cmdline *self);


#endif

