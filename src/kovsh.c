#include "kovsh.h"

#include "lexer.c"
#include "strv.c"
#include "context.c"

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static KshCommand *get_cmd(StrView name);



static KshCommands commands = {0};



void ksh_use_commands_(size_t count, KshCommand cmds[count])
{
    assert(count);
    commands.items = cmds;
    commands.count = count;
}

// TODO: add a parameter as a command result
KshErr ksh_parse(StrView sv)
{
    assert(commands.count);

    Token tok;
    KshCommand *command;
    KshContext context = {0};
    Lexer lex = ksh_lexer_new(sv);

    if (!ksh_lexer_next_token(&lex, &tok)) return KSH_ERR_OK;

    command = get_cmd(tok);
    if (!command) return KSH_ERR_COMMAND_NOT_FOUND;

    context.lex = &lex;
    command->fn(context);
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

static int print(KshContext ctx)
{
    KshErr err = ksh_ctx_init(&ctx, 2);
    if (err != KSH_ERR_OK) return err;

    StrView msg;
    if (!ksh_ctx_get_param(&ctx, STRV_LIT("msg"), KSH_PARAM_TYPE_STR, &msg))
        return 1;

    int repeat;
    if (!ksh_ctx_get_param(&ctx, STRV_LIT("rep"), KSH_PARAM_TYPE_INT, &repeat))
        repeat = 1;

    printf(STRV_FMT"\n", STRV_ARG(msg));

    return 0;
}

int main()
{
    ksh_use_commands({STRV_LIT_DYN("print"), print});
    KshErr err = ksh_parse(STRV_LIT_DYN("print --msg 'Hello, World' --rep"));
    if (err != 0) fprintf(stderr, "ERROR: %d\n", err);

    return err;
}

