#include "kovsh.h"
#include <string.h>
#include <assert.h>

#ifndef MAX_COMMANDS_LEN
#   define MAX_COMMANDS_LEN 100
#endif

static Command commands[MAX_COMMANDS_LEN] = {0};
static int command_cursor = 0;

Command *ksh_commands_add(Command cmd)
{
    assert(command_cursor < MAX_COMMANDS_LEN);
    commands[command_cursor] = cmd;
    return &commands[command_cursor++];
}

void ksh_command_print(Command cmd)
{
    printf("COMMAND: ");
    strslice_print(cmd.name, stdout);
    putchar('\n');
    printf("DESCRIPTION: %s\n", cmd.desc);
    for (size_t i = 0; i < cmd.expected_args_len; i++) {
        ksh_commandarg_print(cmd.expected_args[i]);
    }
}

Command *ksh_command_find(StrSlice ss)
{
    for (size_t i = 0; i < MAX_COMMANDS_LEN; i++) {
        if (strslice_cmp(commands[i].name, ss) == 0) {
            return &commands[i];
        }
    }

    return NULL;
}

CommandArg *ksh_commandarg_find(size_t len, CommandArg args[len], StrSlice ss)
{
    for (size_t i = 0; i < len; i++) {
        if (strslice_cmp(args[i].name, ss) == 0) {
            return &args[i];
        }
    }

    return NULL;
}

void ksh_commandarg_print(CommandArg arg)
{
    printf("ARG: ");
    strslice_print(arg.name, stdout);
    printf(". Type: %s. Usage: %s\n",
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
        .argc = cmd->expected_args_len,
        .argv = cmd->expected_args
    };
}

void ksh_commandcall_set_arg(CommandCall *cmd_call, StrSlice arg_name, CommandArgVal value)
{
    CommandArg *arg = ksh_commandarg_find(cmd_call->argc, cmd_call->argv, arg_name);
    if (arg == NULL) {
        fputs("ERROR: ", stderr);
        strslice_print(arg_name, stderr);
        fputs(": arg not found\n", stderr);
    }

    arg->value = value;
}

void ksh_commandcall_exec(CommandCall cmd_call)
{
    cmd_call.fn(cmd_call.argc, cmd_call.argv);
}
