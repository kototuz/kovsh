#include "cmd.h" 
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef CMD_MAX_CAP
#   define CMD_MAX_CAP 256
#endif
static struct {
    size_t len;
    Cmd list[CMD_MAX_CAP];
} cmds = {0};

static int default_cb_run(void *self) {(void)self; return 0;}
static const CmdCb default_cb = {
    .ctx = NULL,
    .run = default_cb_run
};

static void set_def_if_nothing(Cmd *cmd) {
    assert(cmd->name != 0);
    if (cmd->desc == 0) {
        cmd->desc = "None";
    } else if (cmd->callback.run == 0) {
        cmd->callback = default_cb;
    }
}

Cmd *cmd_init(Cmd snip) {
    set_def_if_nothing(&snip);
    cmds.list[cmds.len] = snip;

    return &cmds.list[cmds.len++];
}

int cmd_run(Cmd *self) {
    return self->callback.run(self->callback.ctx);
}

int cmd_cb_printer_run(const char *msg) {
    printf("Callback message: %s\n", msg);
    return 0;
}

int cmd_strarg_parser(CmdArg *arg, const char *ctx) {
    arg->as_str = ctx;
}

int cmd_intarg_parser(CmdArg *arg, const char *ctx) {
    int num = atoi(ctx);
    if (!num) {
        return -1;
    }
    arg->as_int = num;
}


int cmd_cb_with_args_run(CmdCbWithArgs *self) {
    CmdArgList arg_list = self->arg_list;
    for (size_t i = 0; i < arg_list.len; i++) {
        if (arg_list.data[i].as_int == 0) {
            return -1;
        }
    }

    self->handler(self);
    return 0;
}

static void test(void) {
    void *some = (void *)"Hello, World";
    _Generic(some, char *: puts("Hello, World"));
}

static void handler(CmdCbWithArgs *callb) {
    puts(callb->arg_list.data[0].as_str);
}
int main(void) {
    CmdArg args[] = {
        (CmdArg){
            .name = "msg",
            .parser = cmd_strarg_parser,
            .as_str = "Hello, from kovsh"
        }
    };
    CmdCbWithArgs cb_arg =
        (CmdCbWithArgs){CMD_ARG_LIST(args), handler};
    CmdCb callback = CMD_CB(&cb_arg);
    Cmd *cmd =
        cmd_init((Cmd){.name = "test", .callback = callback});
    cmd_run(cmd);

    return 0;
}
