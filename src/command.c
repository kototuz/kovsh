#include "kovsh.h"
#include <string.h>
#include <assert.h>

#ifndef MAX_COMMANDS_LEN
#   define MAX_COMMANDS_LEN 100
#endif

static Command commands[MAX_COMMANDS_LEN] = {0};
static int command_cursor = 0;

Command ksh_command_new(CommandOpt opt)
{
    assert(opt.name != 0);
    assert(opt.fn != 0);

    return (Command){
        .name = strv_new(opt.name, strlen(opt.name)),
        .desc = opt.desc ? opt.desc : "None",
        .fn = opt.fn,
        .argv = opt.argv,
        .argc = opt.argc
    };
}

CommandArg ksh_commandarg_new(CommandArgOpt opt)
{
    assert(opt.name != 0);

    return (CommandArg) {
        .name = strv_new(opt.name, strlen(opt.name)),
        .usage = opt.usage ? opt.usage : "None",
        .value = opt.value,
    };
}

Command *ksh_commands_add(Command cmd)
{
    assert(command_cursor < MAX_COMMANDS_LEN);
    commands[command_cursor] = cmd;
    return &commands[command_cursor++];
}

void ksh_command_print(Command cmd)
{
    printf("[COMMAND]:\t"STRV_FMT"\n", STRV_ARG(cmd.name));
    printf("[DESCRIPTION]:\t%s\n", cmd.desc);
    puts("[ARGS]:");
    for (size_t i = 0; i < cmd.argc; i++) {
        ksh_commandarg_print(cmd.argv[i]);
    }
}

Command *ksh_command_find(StrView sv)
{
    for (size_t i = 0; i < MAX_COMMANDS_LEN; i++) {
        if (strv_eq(sv, commands[i].name)) {
            return &commands[i];
        }
    }

    return NULL;
}

CommandArg *ksh_commandarg_find(size_t len, CommandArg args[len], StrView ss)
{
    for (size_t i = 0; i < len; i++) {
        if (strv_eq(ss, args[i].name)) {
            return &args[i];
        }
    }

    return NULL;
}

void ksh_commandarg_print(CommandArg arg)
{
    printf("\t"STRV_FMT"=<%s>\t%s\n",
           STRV_ARG(arg.name),
           ksh_commandargvalkind_to_str(arg.value.kind),
           arg.usage);
}

const char *ksh_commandargvalkind_to_str(CommandArgValKind cavt)
{
    switch (cavt) {
    case COMMAND_ARG_VAL_KIND_STR: return "string";
    case COMMAND_ARG_VAL_KIND_INT: return "integer";
    case COMMAND_ARG_VAL_KIND_BOOL: return "bool";
    default: return "unknown";
    }
}

CommandCall ksh_command_create_call(Command *cmd)
{
    return (CommandCall){
        .fn = cmd->fn,
        .argc = cmd->argc,
        .argv = cmd->argv
    };
}

KshErr ksh_commandcall_set_arg(CommandCall *cmd_call, StrView arg_name, CommandArgVal value)
{
    CommandArg *arg = ksh_commandarg_find(cmd_call->argc, cmd_call->argv, arg_name);
    if (arg == NULL) {
        KSH_LOG_ERR("arg not found: "STRV_FMT, STRV_ARG(arg_name));
        return KSH_ERR_ARG_NOT_FOUND;
    }

    if (arg->value.kind != value.kind) {
        KSH_LOG_ERR("the `"STRV_FMT"` argument type is %s",
                    STRV_ARG(arg->name),
                    ksh_commandargvalkind_to_str(arg->value.kind));
        return KSH_ERR_TYPE_EXPECTED;
    }

    arg->value = value;
    return KSH_ERR_OK;
}

void ksh_commandcall_exec(CommandCall cmd_call)
{
    cmd_call.fn(cmd_call.argc, cmd_call.argv);
}
