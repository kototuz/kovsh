#include "kovsh.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifndef DYN_VARS_CAP 
#   define DYN_VARS_CAP 16
#endif

static Variable *find_var(StrView name);
static Variable *find_dyn_var(StrView name);
static KshErr    add_dyn_var(StrView name, StrView value);
static KshErr    set_dyn_var(StrView name, StrView value);

static int builtin_list_var(ArgValueCopy *args);
static int builtin_set_var(ArgValueCopy *args);
static int builtin_print(ArgValueCopy *args);

static KshErr cmd_eval(Lexer *lex, CommandCall *cmd_call);
static KshErr args_eval(Lexer *lex, CommandCall *cmd_call);



static CommandSet builtin_commands = {
    .len = 2,
    .items = (Command[]){
        {
            .name = STRV_LIT("var"),
            .desc = "Lists variables",
            .fn = builtin_list_var,
            .subcommands.len = 1,
            .subcommands.items = (Command[]){
                {
                    .name = STRV_LIT("set"),
                    .desc = "Sets a variable or create a new one",
                    .fn = builtin_set_var,
                    .args_len = 2,
                    .args = (Arg[]){
                        {
                            .name = STRV_LIT("name"),
                            .usage = "Variable name",
                            .type_inst.type_tag = KSH_VALUE_TYPE_TAG_STR
                        },
                        {
                            .name = STRV_LIT("value"),
                            .usage = "Variable value",
                            .type_inst.type_tag = KSH_VALUE_TYPE_TAG_STR
                        }
                    }
                }
            }
        },
        {
            .name = STRV_LIT("print"),
            .desc = "Prints messages",
            .fn = builtin_print,
            .args_len = 1,
            .args = (Arg[]){
                {
                    .name = STRV_LIT("msg"),
                    .usage = "Message to print",
                    .type_inst.type_tag = KSH_VALUE_TYPE_TAG_STR
                }
            }
        }
    }
};

static CommandSet commands[2] = {0};
static VariableSet variables = {0};

static Variable    dynamic_variables_arr[DYN_VARS_CAP];
static VariableSet dynamic_variables = { .items = dynamic_variables_arr };



void ksh_init(void)
{
}

void ksh_deinit(void)
{
    for (size_t i = 0; i < dynamic_variables.len; i++) {
        free(dynamic_variables.items[i].value.items);
        free(dynamic_variables.items[i].name.items);
    }
}

KshErr ksh_parse(StrView sv, CommandCall *dest)
{
    KshErr err;
    Lexer lex = ksh_lexer_new(sv);

    err = cmd_eval(&lex, dest);
    if (err != KSH_ERR_OK) return err;
    err = args_eval(&lex, dest);

    return err;
}

void ksh_use_command_set(CommandSet set)
{
    commands[0] = set;
}

void ksh_use_builtin_commands(void)
{
    commands[1] = builtin_commands;
}

void ksh_use_variable_set(VariableSet set)
{
    variables = set;
}

KshErr ksh_var_get_val(StrView name, StrView *dest)
{
    Variable *var = find_var(name);
    if (!var) var = find_dyn_var(name);
    if (!var) return KSH_ERR_VAR_NOT_FOUND;

    dest->items = var->value.items;
    dest->len = var->value.len;

    return KSH_ERR_OK;
}

static KshErr set_dyn_var(StrView name, StrView value)
{
    Variable *var = find_dyn_var(name);
    if (!var) return KSH_ERR_VAR_NOT_FOUND;

    var->value.items = (char *) realloc(var->value.items, value.len);
    if (!var->value.items) return KSH_ERR_MEM_OVER;

    memcpy(var->value.items, value.items, value.len);
    var->value.len = value.len;

    return KSH_ERR_OK;
}



static Variable *find_var(StrView name)
{
    for (size_t i = 0; i < variables.len; i++)
        if (strv_eq(name, variables.items[i].name))
            return &variables.items[i];

    return NULL;
}

static Variable *find_dyn_var(StrView name)
{
    for (size_t i = 0; i < dynamic_variables.len; i++) {
        if (strv_eq(name, dynamic_variables.items[i].name)) {
            return &dynamic_variables.items[i];
        }
    }

    return NULL;
}

static KshErr add_dyn_var(StrView name, StrView value)
{
    Variable *existed_var;

    if (dynamic_variables.len == DYN_VARS_CAP) return KSH_ERR_MEM_OVER;

    existed_var = find_var(name);
    if (existed_var) return KSH_ERR_NAME_ALREADY_EXISTS;

    existed_var = find_dyn_var(name);
    if (existed_var) return KSH_ERR_NAME_ALREADY_EXISTS;

    char *name_buf = (char *) malloc(name.len);
    char *value_buf = (char *) malloc(value.len);
    if (!name_buf | !value_buf) return KSH_ERR_MEM_OVER;

    memcpy(name_buf, name.items, name.len);
    memcpy(value_buf, value.items, value.len);

    dynamic_variables.items[dynamic_variables.len++] = (Variable){
        .name.items = name_buf,
        .name.len = name.len,
        .value.items = value_buf,
        .value.len = value.len
    };

    return KSH_ERR_OK;
}

static KshErr cmd_eval(Lexer *l, CommandCall *cmd_call)
{
    Token tok;
    Command *subcmd;
    KshErr err;

    err = ksh_lexer_expect_next_token(l, TOKEN_TYPE_LIT, &tok);
    if (err != KSH_ERR_OK) return err;

    Command *cmd;
    cmd = ksh_cmd_find(commands[0], tok.text);
    if (!cmd) cmd = ksh_cmd_find(commands[1], tok.text);
    if (!cmd) {
        KSH_LOG_ERR("command not found: `"STRV_FMT"`", STRV_ARG(tok.text));
        return KSH_ERR_COMMAND_NOT_FOUND;
    }

    if (cmd->subcommands.len > 0) 
        while (ksh_lexer_peek_token(l, &tok) &&
               tok.type == TOKEN_TYPE_LIT) {
            subcmd = ksh_cmd_find(cmd->subcommands, tok.text);
            if (!subcmd) break;

            ksh_lexer_next_token(l, &tok);
            cmd = subcmd;
        }

    err = ksh_cmd_init_call(cmd, cmd_call);
    return err;
}

static KshErr args_eval(Lexer *lex, CommandCall *cmd_call)
{
    (void) exit;
    assert(cmd_call->fn);

    KshErr err;
    ArgValueCopy *arg;
    Token arg_name;
    Token arg_val;
    while (ksh_lexer_peek_token(lex, &arg_name) &&
           arg_name.type != TOKEN_TYPE_PLUS) {
        ksh_lexer_next_token(lex, &arg_name);
        if (arg_name.type == TOKEN_TYPE_LIT &&
            ksh_lexer_next_token_if(lex, TOKEN_TYPE_EQ, &arg_val) &&
            ksh_lexer_next_token(lex, &arg_val))
        {
            arg = ksh_cmd_call_find_value(cmd_call, arg_name.text);
            cmd_call->last_assigned_idx = 
                &cmd_call->args[cmd_call->last_assigned_idx] - cmd_call->args + 1;
        } else {
            if (cmd_call->last_assigned_idx >= cmd_call->args_len) {
                KSH_LOG_ERR("last arg not found%s", "");
                return KSH_ERR_ARG_NOT_FOUND;
            }
            arg_val = arg_name;
            arg = &cmd_call->args[cmd_call->last_assigned_idx++];
        }

        if (arg == NULL) {
            KSH_LOG_ERR("arg not found: `"STRV_FMT"`", STRV_ARG(arg_name.text));
            return KSH_ERR_ARG_NOT_FOUND;
        }

        err = ksh_token_actual(&arg_val);
        if (err != KSH_ERR_OK) return err;

        if (!ksh_val_type_eq_ttype(arg->arg_ptr->type_inst.type_tag, arg_val.type))
            return KSH_ERR_TYPE_EXPECTED;

        err = ksh_val_parse(arg_val.text, arg->arg_ptr->type_inst, &arg->value.data);
        if (err != KSH_ERR_OK) return err;

        arg->value.is_assigned = true;
    }

    return KSH_ERR_OK;
}

static int builtin_list_var(ArgValueCopy *args)
{
    (void) args;

    for (size_t i = 0; i < dynamic_variables.len; i++) {
        printf(STRV_FMT"\n", STRV_ARG(dynamic_variables.items[i].name));
    }
    for (size_t i = 0; i < variables.len; i++) {
        printf(STRV_FMT"\n", STRV_ARG(variables.items[i].name));
    }

    return 0;
}

static int builtin_set_var(ArgValueCopy *args)
{
    KshErr err;
    err = set_dyn_var(args[0].value.data.as_str,
                      args[1].value.data.as_str);
    if (err == KSH_ERR_VAR_NOT_FOUND) {
        err = add_dyn_var(args[0].value.data.as_str,
                          args[1].value.data.as_str);
    }

    return err;
}

static int builtin_print(ArgValueCopy *args)
{
    printf(STRV_FMT"\n", STRV_ARG(args[0].value.data.as_str));
    return 0;
}
