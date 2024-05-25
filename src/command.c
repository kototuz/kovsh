#include "kovsh.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <ncurses.h>

static int echo_fn(size_t argc, Arg argv[argc]);
static int clear_fn(size_t argc, Arg argv[argc]);
static int help_fn(size_t argc, Arg argv[argc]);

#define HARDCODED_COMMANDS_COUNT (sizeof(hardcoded_commands) / sizeof(hardcoded_commands[0]))
static Command hardcoded_commands[] = {
   { // echo
        .name = STRV_LIT("echo"),
        .fn = echo_fn,
        .arg_defs_len = 2,
        .arg_defs = (ArgDef[]){
            {
                .name = STRV_LIT("msg"),
                .type = ARG_VAL_TYPE_STR
            },
            {
                .name = STRV_LIT("rep"),
                .type = ARG_VAL_TYPE_INT,
                .has_default = true,
                .default_val.as_int = 1
            }
        }
   },
   { // clear
      .name = STRV_LIT("clear"),
      .fn = clear_fn,
   },
   { // help
      .name = STRV_LIT("help"),
      .fn = help_fn
   }
};

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
    puts("[ARGS]:");
    for (size_t i = 0; i < cmd.arg_defs_len; i++) {
        ksh_arg_def_print(cmd.arg_defs[i]);
    }
}

Command *ksh_cmd_find_local(CommandBuf buf, StrView sv)
{
    for (size_t i = 0; i < buf.len; i++) {
        if (strv_eq(sv, buf.items[i].name)) {
            return &buf.items[i];
        }
    }

    return NULL;
}

Command *ksh_cmd_find_hardcoded(StrView sv)
{
    for (size_t i = 0; i < HARDCODED_COMMANDS_COUNT; i++) {
        if (strv_eq(sv, hardcoded_commands[i].name)) {
            return &hardcoded_commands[i];
        }
    }

    return NULL;
}

Command *ksh_cmd_find(CommandBuf buf, StrView sv)
{
    Command *cmd = ksh_cmd_find_hardcoded(sv);
    if (cmd) return cmd;
    return ksh_cmd_find_local(buf, sv);
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

// commands
#ifdef TERMIO_NCURSES
static int echo_fn(size_t argc, Arg argv[argc])
{
    for (int i = 0; i < argv[1].value.as_int; i++) {
        ksh_termio_print((TermTextPrefs){0}, STRV_FMT"\n", STRV_ARG(argv[0].value.as_str));
    }

    return 0;
}

static int clear_fn(size_t argc, Arg argv[argc])
{
    (void)argc;
    (void)argv;
    clear();
    return 0;
}

static int help_fn(size_t argc, Arg argv[argc])
{
    (void)argc;
    (void)argv;
    ksh_termio_print(
        (TermTextPrefs){
            .fg_color = TERM_COLOR_YELLOW,
            .mode.bold = true,
        },
        "KOVSH is a terminal shell written in C\n"
    );
    return 0;
}
#endif // TERMIO_NCURSES
