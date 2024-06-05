#include "kovsh.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUILTIN_VARIABLES_LEN 1
#define VARIABLES_LEN (BUILTIN_VARIABLES_LEN + 128)

#define BUILTIN_COMMANDS_COUNT 6
#ifndef USER_COMMANDS_COUNT
#  define USER_COMMANDS_COUNT 128
#endif
#define COMMANDS_COUNT BUILTIN_COMMANDS_COUNT + USER_COMMANDS_COUNT

#define MAX_LINE 100
#define MAX_USER_NAME 10

// embedded commands
static int ksh_clear(KshValue *args);
static int ksh_help(KshValue *args);
static int ksh_exit(KshValue *args);
static int ksh_setmod(KshValue *args);
static int ksh_echo(KshValue *args);
static int ksh_set_var(KshValue *args);

static void init_builtin_variables(void);

static Variable *find_var(StrView name);



static int command_cursor = BUILTIN_COMMANDS_COUNT;
static Command commands[COMMANDS_COUNT] = {
    {
        .name = STRV_LIT("clear"),
        .desc = "Clear terminal",
        .fn = ksh_clear
    },
    {
        .name = STRV_LIT("exit"),
        .desc = "Exit from the terminal",
        .fn = ksh_exit
    },
    {
        .name = STRV_LIT("help"),
        .desc = "Do you want to know about the cmd?",
        .fn = ksh_help,
        .arg_defs_len = 1,
        .arg_defs = (ArgDef[]){
            {
                .name = STRV_LIT("cmd"),
                .usage = "Command",
                .type = KSH_VALUE_TYPE_STR
            }
        }
    },
    {
        .name = STRV_LIT("setmod"),
        .desc = "Set the terminal mod (sys,def)",
        .fn = ksh_setmod,
        .arg_defs_len = 1,
        .arg_defs = (ArgDef[]){
            {
                .name = STRV_LIT("mod"),
                .usage = "Mod to set",
                .type = KSH_VALUE_TYPE_STR
            }
        }
    },
    {
        .name = STRV_LIT("echo"),
        .desc = "Talks with you",
        .fn = ksh_echo,
        .arg_defs_len = 2,
        .arg_defs = (ArgDef[]){
            {
                .name = STRV_LIT("msg"),
                .usage = "Message to print",
                .type = KSH_VALUE_TYPE_STR,
                .has_default = true,
                .default_val.as_str = STRV_LIT("сухарики")
            },
            {
                .name = STRV_LIT("rep"),
                .usage = "Repeat count",
                .type = KSH_VALUE_TYPE_INT,
                .has_default = true,
                .default_val.as_int = 1,
            }
        }
    },
    {
        .name = STRV_LIT("set"),
        .desc = "Set a variable",
        .fn = ksh_set_var,
        .arg_defs_len = 2,
        .arg_defs = (ArgDef[]){
            {
                .name = STRV_LIT("name"),
                .usage = "The variable name",
                .type = KSH_VALUE_TYPE_STR
            },
            {
                .name = STRV_LIT("value"),
                .usage = "The variable value",
                .type = KSH_VALUE_TYPE_ANY
            }
        }
    }
};

static Terminal terminal = {
    .commands.items = commands,
    .commands.len = COMMANDS_COUNT
};

static int variable_cursor = 0;
static Variable variables[VARIABLES_LEN];

KshErr ksh_var_add(Variable var)
{
    if (variable_cursor+1 >= VARIABLES_LEN)
        return KSH_ERR_MEM_OVER;

    variables[variable_cursor++] = var;
    return KSH_ERR_OK;
}

KshErr ksh_var_get_val(StrView name, StrView *dest)
{
    Variable *var = find_var(name);
    if (!var)
        return KSH_ERR_VAR_NOT_FOUND;

    dest->items = var->value.items;
    dest->len = var->value.len;
    return KSH_ERR_OK;
}

KshErr ksh_var_set_val(StrView name, StrView value)
{
    Variable *var = find_var(name);
    if (!var) {
        KSH_LOG_ERR("variable not found: `"STRV_FMT"`", STRV_ARG(name));
        return KSH_ERR_VAR_NOT_FOUND;
    } else if (!var->is_mutable) {
        KSH_LOG_ERR("variable `"STRV_FMT"` is not mutable",
                    STRV_ARG(var->name));
        return KSH_ERR_VAR_NOT_FOUND;
    }

    var->value.items = (char *) realloc(var->value.items, value.len);
    if (!var->value.items) {
        KSH_LOG_ERR("could not allocate memory%s", "");
        return KSH_ERR_MEM_OVER;
    }

    memcpy(var->value.items, value.items, value.len);
    var->value.len = value.len;
    return KSH_ERR_OK;
}

void ksh_term_add_command(Command cmd)
{
    assert(command_cursor+1 < COMMANDS_COUNT);
    assert(cmd.name.items);

    for (size_t i = 0; i < cmd.arg_defs_len; i++) {
        assert(cmd.arg_defs[i].name.items);
        if (!cmd.arg_defs[i].usage) {
            cmd.arg_defs[i].usage = "None";
        }
    }

    if (!cmd.desc) cmd.desc = "None";

    commands[command_cursor++] = cmd;
}

void ksh_term_set_prompt(Prompt p) { terminal.prompt = p; }
void ksh_term_set_mod(TerminalMod mod) { terminal.mod = mod; }

void ksh_prompt_print(Prompt p)
{
    for (size_t i = 0; i < p.parts_len; i++) {
        PromptPart part = p.parts[i];
        TermTextPrefs prefs = p.global_prefs;
        if (part.overrides_global) {
            prefs = part.text_prefs;
        }

        ksh_termio_print(prefs, part.text);
    }
}

static Variable *find_var(StrView name)
{
    for (int i = 0; i < VARIABLES_LEN; i++) {
        if (strv_eq(name, variables[i].name))
            return &variables[i];
    }

    return NULL;
}

void ksh_term_run(void)
{
    init_builtin_variables();
    ksh_termio_init();

    while (!terminal.should_exit) {
        ksh_prompt_print(terminal.prompt);

        char line[MAX_LINE];
        ksh_termio_getline(MAX_LINE, line);

        switch (terminal.mod) {
            case TERMINAL_MOD_DEF:;
                Lexer lex = ksh_lexer_new((StrView){ .items = line, .len = strlen(line) });
                ksh_parse(&lex, &terminal);
                break;
            case TERMINAL_MOD_SYS:
                if (strncmp(line, "@q", 2) == 0) {
                    terminal.mod = TERMINAL_MOD_DEF;
                } else {
                    system(line);
                }
                break;
            default: assert(0 && "unknown terminal mod");
        }

    }

    ksh_termio_end();
}

static void init_builtin_variables(void)
{
    static char user_name[MAX_USER_NAME];
    getlogin_r(user_name, MAX_USER_NAME);
    ksh_var_add((Variable){
        .name = STRV_LIT("user"),
        .value.items = user_name,
        .value.len = strlen(user_name)
    });
}

// embedded commands
#if TERMIO == TERMIO_DEFAULT
static int
ksh_clear(KshValue *args)
{
    (void) args;
    system("clear");
    return 0;
}
#elif TERMIO == TERMIO_NCURSES
#include <ncurses.h>
static int ksh_clear(KshValue *args)
{
    (void) args;
    clear();
    return 0;
}
#endif

static int
ksh_help(KshValue *args)
{
    Command *cmd = ksh_cmd_find(terminal.commands, args[0].as_str);
    if (!cmd) {
        ksh_termio_print((TermTextPrefs){0},
                         STRV_FMT" doesn't exist\n",
                         STRV_ARG(args[0].as_str));
        return 1;
    }

    ksh_cmd_print(*cmd);
    return 0;
}

static int
ksh_exit(KshValue *args)
{
    (void) args;
    terminal.should_exit = true;
    return 0;
}

static int
ksh_setmod(KshValue *args)
{
    if (!strv_eq(args[0].as_str, (StrView)STRV_LIT("sys"))) return 1;
    terminal.mod = TERMINAL_MOD_SYS;
    return 0;
}

static int
ksh_echo(KshValue *args)
{
    for (int i = 0; i < args[1].as_int; i++) {
        ksh_termio_print((TermTextPrefs){0},
                         STRV_FMT"\n",
                         STRV_ARG(args[0].as_str));
    }

    return 0;
}

static int ksh_set_var(KshValue *args)
{
    StrView name = args[0].as_str;
    StrView value = args[1].as_str;

    Variable *var = find_var(name);
    if (var)
        return ksh_var_set_val(name, value);

    char *name_buf = (char *) malloc(name.len);
    char *value_buf = (char *) malloc(value.len);

    memcpy(name_buf, name.items, name.len);
    memcpy(value_buf, value.items, value.len);

    Variable new_var = {
        .name.items = name_buf,
        .name.len = name.len,
        .value.items = value_buf,
        .value.len = value.len,
        .is_mutable = true
    };

    if (ksh_var_add(new_var) != KSH_ERR_OK)
        return 1;

    return 0;
}
