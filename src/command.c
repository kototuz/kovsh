#include "kovsh.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

static KshErr handle_arg_assignment(Arg *arg)
{
    if (!arg->is_assign) {
        if (arg->def->has_default) {
            arg->value = arg->def->default_val;
        } else {
            KSH_LOG_ERR("arg `"STRV_FMT"` must be assigned", STRV_ARG(arg->def->name));
            return KSH_ERR_ASSIGNMENT_EXPECTED;
        }
    }

    return KSH_ERR_OK;
}

Arg *cmd_call_find_arg(CommandCall cmd_call, StrView arg_name)
{
    for (size_t i = 0; i < cmd_call.argc; i++) {
        if (strv_eq(cmd_call.argv[i].def->name, arg_name)) {
            return &cmd_call.argv[i];
        }
    }

    return NULL;
}

void ksh_cmd_print(Command cmd)
{
    printf("[COMMAND]:\t"STRV_FMT"\n", STRV_ARG(cmd.name));
    printf("[DESCRIPTION]:\t%s\n", cmd.desc);

    if (cmd.arg_defs_len < 1) return;

    puts("[ARGS]:%s");
    for (size_t i = 0; i < cmd.arg_defs_len; i++) {
        printf("\t"STRV_FMT"=<%s>\t%s\n",
               STRV_ARG(cmd.arg_defs[i].name),
               ksh_val_type_str(cmd.arg_defs[i].type),
               cmd.arg_defs[i].usage);
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

ArgDef *ksh_cmd_find_arg_def(Command *cmd, StrView sv)
{
    ArgDef *arg_defs = cmd->arg_defs;
    size_t arg_defs_len = cmd->arg_defs_len;
    for (size_t i = 0; i < arg_defs_len; i++) {
        if (strv_eq(sv, arg_defs[i].name)) {
            return &arg_defs[i];
        }
    }

    return NULL;
}

CommandCall ksh_cmd_create_call(Command *cmd)
{
    size_t args_len = cmd->arg_defs_len;
    CommandCall res = {
        .cmd = cmd,
        .argv = malloc(sizeof(Arg) * args_len),
        .argc = args_len
    };

    if (res.argv == NULL) {
        fputs("ERROR: could not allocate memmory\n", stderr);
        exit(1);
    }

    ArgDef *arg_defs = cmd->arg_defs;
    for (size_t i = 0; i < args_len; i++) {
        res.argv[i] = (Arg){ .def = &arg_defs[i] };
    }

    return res;
}

KshErr ksh_cmd_call_exec(CommandCall cmd_call)
{
    for (size_t i = 0; i < cmd_call.argc; i++) {
        KshErr err = handle_arg_assignment(&cmd_call.argv[i]);
        if (err != KSH_ERR_OK) return err;
    }

    KshValue *args = (KshValue *) malloc(sizeof(*args) * cmd_call.argc);
    if (!args) {
        KSH_LOG_ERR("could not allocate memory%s", "");
        return KSH_ERR_MEM_OVER;
    }

    for (size_t i = 0; i < cmd_call.argc; i++) {
        args[i] = cmd_call.argv[i].value;
    }

    cmd_call.cmd->fn(args);

    free(args);
    free(cmd_call.argv);
    return KSH_ERR_OK;
}

Arg *ksh_args_find(size_t argc, Arg argv[argc], StrView sv)
{
    for (size_t i = 0; i < argc; i++) {
        if (strv_eq(argv[i].def->name, sv)) {
            return &argv[i];
        }
    }

    return NULL;
}
