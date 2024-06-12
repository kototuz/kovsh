#include "kovsh.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static Variable *find_var(StrView name);
static Variable *find_usr_var(StrView name);

static KshErr cmd_eval(Lexer *lex, CommandCall *cmd_call);
static KshErr args_eval(Lexer *lex, CommandCall *cmd_call);



static CommandSet commands = {0};
static VariableSet variables = {0};



void ksh_init(void)
{
}

void ksh_deinit(void)
{
    for (size_t i = 0; i < variables.len; i++)
        free(variables.items[i].value.items);
}

KshErr ksh_parse(StrView sv, CommandCall *dest)
{
    KshErr err;
    Lexer lex = ksh_lexer_new(sv);

    err = cmd_eval(&lex, dest);
    if (err != KSH_ERR_OK) return err;
    err = args_eval(&lex, dest);

    return KSH_ERR_OK;
}

void ksh_use_command_set(CommandSet set)
{
    commands = set;
}

void ksh_use_variable_set(VariableSet set)
{
    variables = set;
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
    Variable *var = find_usr_var(name);
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

static Variable *find_usr_var(StrView name)
{
    for (size_t i = 0; i < variables.len; i++)
        if (strv_eq(name, variables.items[i].name))
            return &variables.items[i];

    return NULL;
}

static KshErr cmd_eval(Lexer *l, CommandCall *cmd_call)
{
    Token tok;
    Command *subcmd;
    KshErr err;

    err = ksh_lexer_expect_next_token(l, TOKEN_TYPE_LIT, &tok);
    if (err != KSH_ERR_OK) return err;

    Command *cmd = ksh_cmd_find(commands, tok.text);
    if (cmd == NULL) {
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

        KshErr err;
        err = ksh_token_parse_to_value(arg_val, arg->arg_ptr->value_type, &arg->value.data);
        if (err != KSH_ERR_OK) return err;

        err = ksh_val_parse(arg->arg_ptr->value_type, &arg->value.data);
        if (err != KSH_ERR_OK) return err;

        arg->value.is_assigned = true;
    }

    return KSH_ERR_OK;
}
