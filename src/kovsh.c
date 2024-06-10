#include "kovsh.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define BUILTIN_VARS_LEN 1
#define BUILTIN_CMDS_LEN 2

#ifndef USR_CMDS_LEN
#   define USR_CMDS_LEN 128
#endif

#ifndef USR_VARS_LEN
#   define USR_VARS_LEN 128
#endif

#define CMDS_LEN (BUILTIN_CMDS_LEN + USR_CMDS_LEN)
#define VARS_LEN (BUILTIN_VARS_LEN + USR_VARS_LEN)

static void init_builtin_variables(void);

static Variable *find_var(StrView name);
static Variable *find_usr_var(StrView name);

static KshErr cmd_eval(Lexer *lex, CommandCall *cmd_call);
static KshErr args_eval(Lexer *lex, CommandCall *cmd_call);

static int builtin_print(Arg *args);
static int builtin_set_var(Arg *args);
static int builtin_list_vars(Arg *args);
static int builtin_enum_test(Arg *args);

static int command_cursor = BUILTIN_CMDS_LEN;
static Command cmd_arr[CMDS_LEN] = {
    {
        .name = STRV_LIT("print"),
        .desc = "Prints a message",
        .call.fn = builtin_print,
        .call.args_len = 1,
        .call.args = (Arg[]){
            {
                .name = STRV_LIT("msg"),
                .usage = "A message to print",
                .value_type.tag = KSH_VALUE_TYPE_TAG_STR,
            }
        }
    },
    {
        .name = STRV_LIT("var"),
        .desc = "Lists variables",
        .call.fn = builtin_list_vars,
        .subcommands = {
            .len = 1,
            .items = (Command[]){
                {
                    .name = STRV_LIT("set"),
                    .desc = "Sets or creates a variable",
                    .call.fn = builtin_set_var,
                    .call.args_len = 2,
                    .call.args = (Arg[]){
                        {
                            .name = STRV_LIT("name"),
                            .usage = "Variable name",
                            .value_type.tag = KSH_VALUE_TYPE_TAG_STR,
                        },
                        {
                            .name = STRV_LIT("value"),
                            .usage = "Variable value",
                            .value_type.tag = KSH_VALUE_TYPE_TAG_ANY
                        }
                    }
                }
            }
        }
    },
    {
        .name = STRV_LIT("enum"),
        .desc = "Enum test",
        .call.fn = builtin_enum_test,
        .call.args_len = 1,
        .call.args = (Arg[]){
            {
                .name = STRV_LIT("enum"),
                .value_type.tag = KSH_VALUE_TYPE_TAG_ENUM,
                .value_type.info = &(KshValueTypeEnum){
                    .cases = (StrView[]){
                        STRV_LIT("first"),
                        STRV_LIT("second"),
                        STRV_LIT("third")
                    },
                    .cases_len = 3
                }
            }
        }
    }
};

static CommandBuf commands = { .items = cmd_arr, .len = CMDS_LEN };

static int variable_cursor = BUILTIN_VARS_LEN;
static Variable variables[VARS_LEN];



void ksh_init(void)
{
    init_builtin_variables();
}

void ksh_deinit(void)
{
    for (int i = 0; i < VARS_LEN; i++)
        free(variables[i].value.items);
}

KshErr ksh_parse(StrView sv, CommandCall *dest)
{
    KshErr err;
    Lexer lex = ksh_lexer_new(sv);

    err = cmd_eval(&lex, dest);
    if (err != KSH_ERR_OK) return err;
    err = args_eval(&lex, dest);
    if (err != KSH_ERR_OK) return err;

    return KSH_ERR_OK;
}

void ksh_add_command(Command cmd)
{
    assert(command_cursor+1 < CMDS_LEN);
    assert(cmd.name.items);
    assert(cmd.call.fn);

    for (size_t i = 0; i < cmd.call.args_len; i++) {
        assert(cmd.call.args[i].name.items);
        if (!cmd.call.args[i].usage)
            cmd.call.args[i].usage = "none";
    }

    if (!cmd.desc) cmd.desc = "none";

    commands.items[command_cursor++] = cmd;
}

KshErr ksh_var_add(const StrView name, const StrView value)
{
    assert(variable_cursor+1 < VARS_LEN);
    assert(name.items);
    assert(value.items);

    char *name_buf = (char *) malloc(name.len);
    char *value_buf = (char *) malloc(value.len);
    if (!name_buf || !value_buf) return KSH_ERR_MEM_OVER;

    memcpy(name_buf, name.items, name.len);
    memcpy(value_buf, value.items, value.len);

    variables[variable_cursor++] = (Variable){
        .name.items = name_buf,
        .name.len = name.len,
        .value.items = value_buf,
        .value.len = value.len
    };

    return KSH_ERR_OK;
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



static void init_builtin_variables(void)
{
    char *user = (char *) malloc(10);
    assert(user);
    getlogin_r(user, 10);
    variables[0] = (Variable){
        .name = STRV_LIT("user"),
        .value.items = user,
        .value.len = strlen(user)
    };
}

static Variable *find_var(StrView name)
{
    for (int i = 0; i < VARS_LEN; i++)
        if (strv_eq(name, variables[i].name))
            return &variables[i];

    return NULL;
}

static Variable *find_usr_var(StrView name)
{
    for (int i = BUILTIN_VARS_LEN; i < VARS_LEN; i++)
        if (strv_eq(name, variables[i].name))
            return &variables[i];

    return NULL;
}

static KshErr cmd_eval(Lexer *l, CommandCall *cmd_call)
{
    Token tok;
    Command *subcmd;

    KshErr err = ksh_lexer_expect_next_token(l, TOKEN_TYPE_LIT, &tok);
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


    *cmd_call = cmd->call;

    return KSH_ERR_OK;
}

static KshErr args_eval(Lexer *lex, CommandCall *cmd_call)
{
    (void) exit;
    assert(cmd_call->fn);

    Arg *arg;
    Token arg_name;
    Token arg_val;
    while (ksh_lexer_peek_token(lex, &arg_name) &&
           arg_name.type != TOKEN_TYPE_PLUS) {
        ksh_lexer_next_token(lex, &arg_name);
        if (arg_name.type == TOKEN_TYPE_LIT &&
            ksh_lexer_next_token_if(lex, TOKEN_TYPE_EQ, &arg_val) &&
            ksh_lexer_next_token(lex, &arg_val))
        {
            arg = ksh_cmd_call_find_arg(*cmd_call, arg_name.text);
            cmd_call->last_assigned_arg_idx = arg - cmd_call->args + 1;
        } else {
            if (cmd_call->last_assigned_arg_idx >= cmd_call->args_len) {
                KSH_LOG_ERR("last arg not found%s", "");
                return KSH_ERR_ARG_NOT_FOUND;
            }
            arg_val = arg_name;
            arg = &cmd_call->args[cmd_call->last_assigned_arg_idx++];
        }

        if (arg == NULL) {
            KSH_LOG_ERR("arg not found: `"STRV_FMT"`", STRV_ARG(arg_name.text));
            return KSH_ERR_ARG_NOT_FOUND;
        }

        KshErr err;
        if (!ksh_token_type_fit_value_type(arg_val.type, arg->value_type.tag))
            return KSH_ERR_TYPE_EXPECTED;

        err = ksh_token_parse_to_value(arg_val, &arg->value);
        if (err != KSH_ERR_OK) return err;

        err = ksh_val_parse(arg->value_type, &arg->value);
        if (err != KSH_ERR_OK) return err;

        arg->is_assigned = true;
    }

    return KSH_ERR_OK;
}

static int builtin_print(Arg *args)
{
    printf(STRV_FMT"\n", STRV_ARG(args[0].value.as_str));
    return 0;
}

static int builtin_list_vars(Arg *args)
{
    (void) args;
    for (size_t i = 0; i < VARS_LEN; i++) {
        if (!variables[i].name.items) break;
        printf(STRV_FMT"\n", STRV_ARG(variables[i].name));
    }

    return 0;
}

static int builtin_set_var(Arg *args)
{
    StrView name = args[0].value.as_str;
    StrView value = args[1].value.as_str;

    KshErr err = ksh_var_set_val(name, value);
    if (err == KSH_ERR_VAR_NOT_FOUND)
        if (ksh_var_add(name, value) != KSH_ERR_OK)
            return 1;

    return 0;
}

static int builtin_enum_test(Arg *args)
{
    if (args[0].value.as_int == 0) puts("0");
    else if (args[0].value.as_int == 1) puts("1");
    else if (args[0].value.as_int == 2) puts("2");
    return 0;
}
