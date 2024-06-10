#include "kovsh.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

static KshErr handle_arg_assignment(Arg *arg)
{
    if (!arg->is_assigned) {
        KSH_LOG_ERR("arg `"STRV_FMT"` must be assigned", STRV_ARG(arg->name));
        return KSH_ERR_ASSIGNMENT_EXPECTED;
    }

    return KSH_ERR_OK;
}

void ksh_cmd_print(Command cmd)
{
    printf("[COMMAND]:\t"STRV_FMT"\n", STRV_ARG(cmd.name));
    printf("[DESCRIPTION]:\t%s\n", cmd.desc);

    if (cmd.call.args_len < 1) return;

    puts("[ARGS]:%s");
    for (size_t i = 0; i < cmd.call.args_len; i++) {
        printf("\t"STRV_FMT"=<%s>\t%s\n",
               STRV_ARG(cmd.call.args[i].name),
               ksh_val_type_tag_str(cmd.call.args[i].value_type.tag),
               cmd.call.args[i].usage);
    }
}

Command *ksh_cmd_find(CommandBuf buf, StrView sv)
{
    for (size_t i = 0; i < buf.len; i++) {
        if (strv_eq(sv, buf.items[i].name)) {
            return &buf.items[i];
        }
    }

    return NULL;
}

Arg *ksh_cmd_call_find_arg(CommandCall call, StrView sv)
{
    for (size_t i = 0; i < call.args_len; i++) {
        if (strv_eq(sv, call.args[i].name)) {
            return &call.args[i];
        }
    }

    return NULL;
}

KshErr ksh_cmd_call_execute(CommandCall cmd_call)
{
    for (size_t i = 0; i < cmd_call.args_len; i++) {
        KshErr err = handle_arg_assignment(&cmd_call.args[i]);
        if (err != KSH_ERR_OK) return err;
    }

    cmd_call.fn(cmd_call.args);

    return KSH_ERR_OK;
}
