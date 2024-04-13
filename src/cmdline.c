#include "cmd.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>

static char *nextCmdlineToken(char **string) {
    char *ptr = {0};

    while (isspace(**string)) {
        (*string)++;
    }

    if (**string == '\0') {
        return NULL;
    } else {
        ptr = *string;
        while (!isspace(**string)) {
            (*string)++;
        }
        *(*string)++ = '\0';
    }

    return ptr;
}

static Cmd *findCmd(Cmdline *self, const char *token) {
    size_t len = self->config.cmdList.len;
    for (size_t i = 0; i < len; i++) {
        if (strcmp(self->config.cmdList.items[i].name, token) == 0) {
            return &self->config.cmdList.items[i];
        }
    }
    return NULL;
}

static void setCmdArg(Cmd *cmd, const char *token) {
    char argName[10] = {0};
    char argValue[10] = {0};

    sscanf(token, "%s=%s", argName, argValue);

    if (argName[0] != 0 && argValue[0] != 0) {
        size_t argLen = cmd->callback.argList.len;
        for (size_t i = 0; i < argLen; i++) {
            if (strcmp(cmd->callback.argList.items[i].name, argName) == 0) {
                cmd->callback.argList.items[i].parser(&cmd->callback.argList.items[i], argValue);
                return;
            }
        }
    }

    puts("incorrect args initialization");
}

Cmd *cmdlineRequestCmd(Cmdline *self) {
    fprintf(self->output, self->config.reqMsg);

    char buffer[CMDLINE_CAP] = {0};
    fgets(buffer, sizeof(buffer), stdin);

    char *tmp = buffer;
    char *nextToken = nextCmdlineToken(&tmp);
    Cmd *cmd = findCmd(self, nextToken);

    if (cmd) {
        while ((nextToken = nextCmdlineToken(&tmp)) != NULL) {
            setCmdArg(cmd, nextToken);
        }
        return cmd;
    }

    return NULL;
}

static int echoCb(void *self, CmdArgList args) {
    (void)self;
    puts(args.items[0].asStr);
    return 0;
}
int main(void) {
    Cmd cmds[] = {
        (Cmd){.name = "test", .callback = cmdCbPrintCb("Hello, World")},
        (Cmd){
            .name = "echo",
            .callback = (CmdCb){
                .ctx = NULL,
                .run = (CmdCbFn)echoCb,
                .argList = (CmdArgList){
                    .len = 1,
                    .items = (CmdArg[]){
                        (CmdArg){
                            .name = "msg",
                            .parser = CMD_ARG_STR_PARSER,
                            .asStr = "DEFAULT_TEXT"
                        }
                    }
                }
            }
        }
    };

    CmdlineConfig config = {
        "$ ",
        10,
        (CmdList){.len = 2, .items = cmds}
    };
    Cmdline cmdline = {stdout, config};

    Cmd *cmd = cmdlineRequestCmd(&cmdline);
    cmdRun(cmd);
}

