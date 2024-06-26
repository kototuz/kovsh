#include "kovsh.h"

#include "lexer.c"
#include "strv.c"
#include "context.c"

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static KshCommand *get_cmd(StrView name);
static KshErr get_arg_name(Lexer *lex, StrView *res);
static void dump_context(KshContext ctx);



static KshCommands commands = {0};



void ksh_use_commands(KshCommands cmds)
{
    assert(cmds.count);
    assert(cmds.items);
    commands = cmds;
}

KshErr ksh_parse(StrView sv)
{
    assert(commands.count);
    assert(commands.items);

    KshErr err;
    Token tok;
    KshCommand *command;
    KshContext context = {0};
    Lexer lex = ksh_lexer_new(sv);

    if (!ksh_lexer_next_token(&lex, &tok)) return KSH_ERR_OK;

    command = get_cmd(tok);
    if (!command) return KSH_ERR_COMMAND_NOT_FOUND;

    if (!ksh_lexer_peek_token(&lex, &tok)) {
        command->fn(context);
        return KSH_ERR_OK;
    }

    context.args = (KshArg *) malloc(sizeof(KshArg) * command->max_args);
    if (!context.args) return KSH_ERR_MEM_OVER;

    while (ksh_lexer_peek_token(&lex, &tok) && context.args_count < command->max_args) {
        StrView arg_name;
        err = get_arg_name(&lex, &arg_name);
        if (err != KSH_ERR_OK) {
            free(context.args);
            return err; 
        }
        context.args[context.args_count].name = arg_name;

        // TODO: remove `=`
        if (ksh_lexer_peek_token(&lex, &tok) && strv_eq(tok, STRV_LIT_DYN("="))) {
            ksh_lexer_next_token(&lex, &tok);
            if (!ksh_lexer_next_token(&lex, &tok)) {
                free(context.args);
                return KSH_ERR_TOKEN_EXPECTED;
            }
            context.args[context.args_count++].value = tok;
        } else context.args[context.args_count++].value = (StrView){0};
    }

    command->fn(context);
    free(context.args);
    return KSH_ERR_OK;
}



static KshCommand *get_cmd(StrView name)
{
    for (size_t i = 0; i < commands.count; i++) {
        if (strv_eq(commands.items[i].name, name)) {
            return &commands.items[i];
        }
    }

    return NULL;
}

static KshErr get_arg_name(Lexer *lex, StrView *res)
{
    KshErr err;
    Token tok;

    Token s = STRV_LIT("-");
    err = ksh_lexer_expect_next_token(lex, s);
    if (err != KSH_ERR_OK) return err;
    err = ksh_lexer_expect_next_token(lex, s);
    if (err != KSH_ERR_OK) return err;

    if (!ksh_lexer_next_token(lex, &tok)) return KSH_ERR_TOKEN_EXPECTED;

    *res = tok;
    return KSH_ERR_OK;
}

void dump_context(KshContext ctx)
{
    for (size_t i = 0; i < ctx.args_count; i++) {
        printf(STRV_FMT"="STRV_FMT"\n",
               STRV_ARG(ctx.args[i].name),
               STRV_ARG(ctx.args[i].value));
    }
}



static int print(KshContext ctx);
static KshCommands print_cmd = (KshCommands){
    .count = 1,
    .items = (KshCommand[]){
        {
            .name = STRV_LIT("print"),
            .fn = print,
            .max_args = 2,
        }
    }
};
static int print(KshContext ctx)
{
    StrView msg;
    if (!ksh_ctx_get_param(ctx, STRV_LIT_DYN("msg"), KSH_PARAM_TYPE_STR, &msg)) {
        fputs("ERROR: message is required\n", stderr);
        return 1;
    }

    if (ksh_ctx_get_option(ctx, STRV_LIT_DYN("inf")))
        for (;;) printf(STRV_FMT"\n", STRV_ARG(msg));

    printf(STRV_FMT"\n", STRV_ARG(msg));

    return 0;
}
int main()
{
    ksh_use_commands(print_cmd);

    KshErr err = ksh_parse(STRV_LIT_DYN("print --msg='Hello, World'"));
    if (err != 0) fprintf(stderr, "ERROR: %d\n", err);

    return err;
}

