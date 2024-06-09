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

static KshErr cmd_eval(Lexer *lex, CommandCall *cmd_call);
static KshErr args_eval(Lexer *lex, CommandCall *cmd_call);

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
        .value.items = user,
        .value.len = strlen(user)
    };
}

static Variable *find_var(StrView name)
{
    for (int i = 0; i < VARIABLES_LEN; i++)
        if (strv_eq(name, variables[i].name))
            return &variables[i];

    return NULL;
}

static KshErr cmd_eval(Lexer *l, CommandCall *cmd_call)
{
    Token tok;
    KshErr err = ksh_lexer_expect_next_token(l, TOKEN_TYPE_LIT, &tok);
    if (err != KSH_ERR_OK) return err;

    Command *cmd = ksh_cmd_find(commands, tok.text);
    if (cmd == NULL) {
        KSH_LOG_ERR("command not found: `"STRV_FMT"`", STRV_ARG(tok.text));
        return KSH_ERR_COMMAND_NOT_FOUND;
    }

    *cmd_call = ksh_cmd_create_call(cmd);

    return KSH_ERR_OK;
}

static KshErr args_eval(Lexer *lex, CommandCall *cmd_call)
{
    (void) exit;
    assert(cmd_call->cmd);

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
            arg = ksh_args_find(cmd_call->argc,
                                cmd_call->argv,
                                arg_name.text);
            cmd_call->last_assigned_arg_idx = arg - cmd_call->argv + 1;
        } else {
            if (cmd_call->last_assigned_arg_idx >= cmd_call->argc) {
                KSH_LOG_ERR("last arg not found%s", "");
                return KSH_ERR_ARG_NOT_FOUND;
            }
            arg_val = arg_name;
            arg = &cmd_call->argv[cmd_call->last_assigned_arg_idx++];
        }

        if (arg == NULL) {
            KSH_LOG_ERR("arg not found: `"STRV_FMT"`", STRV_ARG(arg_name.text));
            return KSH_ERR_ARG_NOT_FOUND;
        }

        KshErr err = ksh_token_parse_to_value(arg_val, arg->def->type, &arg->value);
        if (err != KSH_ERR_OK) {
            if (err == KSH_ERR_TYPE_EXPECTED) {
                KSH_LOG_ERR("arg `"STRV_FMT"`: expected type: <%s>",
                            STRV_ARG(arg->def->name),
                            ksh_val_type_str(arg->def->type));
            }
            return err;
        }
        arg->is_assign = true;
    }

    return KSH_ERR_OK;
}

static int builtin_print(KshValue *args)
{
    printf(STRV_FMT"\n", STRV_ARG(args[0].as_str));
    return 0;
}
