#include "kovsh.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifndef COMMANDS_LEN
#   define COMMANDS_LEN 128
#endif

#ifndef VARIABLES_LEN
#   define VARIABLES_LEN 128
#endif


static void init_builtin_variables(void);

static Variable *find_var(StrView name);

static int builtin_print(KshValue *args);



#define BUILTIN_CMDS_LEN 2
static int command_cursor = BUILTIN_CMDS_LEN;
static CommandBuf commands = {
    .items = (Command[]){
        {
            .name = STRV_LIT("print"),
            .desc = "Prints a message",
            .fn = builtin_print,
            .arg_defs_len = 1,
            .arg_defs = (ArgDef[]){
                {
                    .name = STRV_LIT("msg"),
                    .type = KSH_VALUE_TYPE_STR
                }
            }
        }
    },
    .len = BUILTIN_CMDS_LEN
};

#define BUILTIN_VARS_LEN 1
static int variable_cursor = BUILTIN_VARS_LEN;
static Variable variables[VARIABLES_LEN];



void ksh_init(void)
{
    init_builtin_variables();
}

void ksh_deinit(void)
{
}

KshErr ksh_parse(StrView sv, CommandCall *dest)
{
}

void ksh_add_command(Command cmd)
{
    assert(command_cursor+1 < COMMANDS_LEN);
    assert(cmd.name.items);
    assert(cmd.fn);

    for (size_t i = 0; i < cmd.arg_defs_len; i++) {
        assert(cmd.arg_defs[i].name.items);
        if (!cmd.arg_defs[i].usage)
            cmd.arg_defs[i].usage = "none";
    }

    if (!cmd.desc) cmd.desc = "none";

    commands.items[command_cursor++] = cmd;
}

void ksh_var_add(Variable var)
{
    assert(variable_cursor+1 < VARIABLES_LEN);
    assert(var.name.items);
    assert(var.value.items);
    variables[variable_cursor++] = var;
}

KshErr ksh_var_get_val(StrView name, StrView *dest)
{
    Variable *var = find_var(name);
    if (!var) return KSH_ERR_VAR_NOT_FOUND;

    dest->items = var->value.items;
    dest->len = var->value.len;

    return KSH_ERR_OK;
}

KshErr ksh_var_set_val(StrView name, StrView value)
{
    Variable *var = find_var(name);
    if (!var || !var->is_mutable) return KSH_ERR_VAR_NOT_FOUND;

    var->value.items = (char *) realloc(var->value.items, value.len);
    if (!var->value.items) return KSH_ERR_MEM_OVER;

    memcpy(var->value.items, value.items, value.len);
    var->value.len = value.len;

    return KSH_ERR_OK;
}



static void init_builtin_variables(void)
{
    char user[10];
    getlogin_r(user, 10);
    variables[0] = (Variable){
        .name = STRV_LIT("user"),
        .value = user
    };
}

static Variable *find_var(StrView name)
{
    for (int i = 0; i < VARIABLES_LEN; i++)
        if (strv_eq(name, variables[i].name))
            return &variables[i];

    return NULL;
}

static int builtin_print(KshValue *args)
{
    printf(STRV_FMT"\n", STRV_ARG(args[0].as_str));
    return 0;
}
