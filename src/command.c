#include "kovsh.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#ifndef MAX_COMMANDS_LEN
#   define MAX_COMMANDS_LEN 100
#endif

static Command cmds[MAX_COMMANDS_LEN] = {0};
static int cmd_cursor = 0;

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

Command *ksh_cmds_add(Command cmd)
{
    assert(cmd_cursor < MAX_COMMANDS_LEN);
    assert(cmd.name.items);
    assert(cmd.fn);

    for (size_t i = 0; i < cmd.arg_defs_len; i++) {
        assert(cmd.arg_defs[i].name.items);
        if (cmd.arg_defs[i].usage == 0) {
            cmd.arg_defs[i].usage = "None";
        }
    }

    if (cmd.desc == 0) {
        cmd.desc = "None";
    }

    cmds[cmd_cursor] = cmd;
    return &cmds[cmd_cursor++];
}

void ksh_cmd_print(Command cmd)
{
    printf("[COMMAND]:\t"STRV_FMT"\n", STRV_ARG(cmd.name));
    printf("[DESCRIPTION]:\t%s\n", cmd.desc);
    puts("[ARGS]:");
    for (size_t i = 0; i < cmd.arg_defs_len; i++) {
        ksh_arg_def_print(cmd.arg_defs[i]);
    }
}

Command *ksh_cmd_find(StrView sv)
{
    for (size_t i = 0; i < MAX_COMMANDS_LEN; i++) {
        if (strv_eq(sv, cmds[i].name)) {
            return &cmds[i];
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

void ksh_arg_def_print(ArgDef arg)
{
    printf("\t"STRV_FMT"=<%s>\t%s\n",
           STRV_ARG(arg.name),
           ksh_arg_val_type_to_str(arg.type),
           arg.usage);
}

const char *ksh_arg_val_type_to_str(ArgValType cavt)
{
    switch (cavt) {
    case ARG_VAL_TYPE_STR: return "string";
    case ARG_VAL_TYPE_INT: return "integer";
    case ARG_VAL_TYPE_BOOL: return "bool";
    default: return "unknown";
    }
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

    cmd_call.cmd->fn(cmd_call.argc, cmd_call.argv);
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
