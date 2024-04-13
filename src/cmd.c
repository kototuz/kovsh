#include "cmd.h" 
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int cmdNoneCallback(void *self, CmdArgList list) {
    (void)self;
    (void)list;
    return 0;
}

int cmdRun(Cmd *cmd) {
    return cmd->callback.run(cmd->callback.ctx,
        cmd->callback.argList);
}

int CMD_ARG_STR_PARSER(CmdArg *arg, const char *ctx) {
    arg->asStr = ctx;
    return 0;
}

int CMD_ARG_INT_PARSER(CmdArg *arg, const char *ctx) {
    int num = atoi(ctx);
    if (!num) {
        return -1;
    }

    arg->asInt = num;
    return 0;
}

int cmdCbPrintCbRun(const char *msg, CmdArgList args) {
    (void)args;
    printf("Message: %s\n", msg);
    return 0;
}
