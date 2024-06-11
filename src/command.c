#include "kovsh.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

static KshErr check_assignment(CommandCallValue *values, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (!values[i].is_assigned) {
            return KSH_ERR_ASSIGNMENT_EXPECTED;
        }
    }

    return KSH_ERR_OK;
}

void ksh_cmd_print(Command cmd)
{
    printf("[COMMAND]:\t"STRV_FMT"\n", STRV_ARG(cmd.name));
    printf("[DESCRIPTION]:\t%s\n", cmd.desc);

    if (cmd.args_len < 1) return;

    puts("[ARGS]:%s");
    for (size_t i = 0; i < cmd.args_len; i++) {
        printf("\t"STRV_FMT"=<%s>\t%s\n",
               STRV_ARG(cmd.args[i].name),
               ksh_val_type_tag_str(cmd.args[i].value_type.tag),
               cmd.args[i].usage);
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

KshErr ksh_cmd_init_call(Command *self, CommandCall *call)
{
    *call = (CommandCall){
        .values = (CommandCallValue *) malloc(sizeof(CommandCallValue) * self->args_len),
        .values_len = self->args_len,
        .fn = self->fn,
    };

    if (!call->values) return KSH_ERR_MEM_OVER;

    for (size_t i = 0; i < self->args_len; i++) {
        call->values[i].arg_def = &self->args[i];
        call->values[i].is_assigned = self->args[i].has_default;
        if (self->args[i].has_default) {
            call->values[i].data = self->args[i].default_value;
            call->values[i].is_assigned = true;
        }
    }

    return KSH_ERR_OK;
}

KshErr ksh_cmd_call_execute(CommandCall cmd_call)
{
    KshErr err = check_assignment(cmd_call.values, cmd_call.values_len);
    if (err != KSH_ERR_OK) return err;

    cmd_call.fn(cmd_call.values);

    return KSH_ERR_OK;
}

CommandCallValue *ksh_cmd_call_find_value(CommandCall *self, StrView name)
{
    for (size_t i = 0; i < self->values_len; i++) {
        if (strv_eq(name, self->values[i].arg_def->name)) {
            return &self->values[i];
        }
    }

    return NULL;
}
