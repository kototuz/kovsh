#include "kovsh.h"

#include "lexer.c"
#include "strv.c"
#include "context.c"
#include "err.c"

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

    KshCommand *command;
    KshContext context = {0};
    Lexer lex = { .text = sv };

    if (!lex_next(&lex)) return KSH_ERR_OK;

    command = get_cmd(lex.cur_tok);
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


static int test(KshContext ctx)
{
    KshErr err = ksh_ctx_init(&ctx, 4);
    if (err != KSH_ERR_OK) return err;

    if (ksh_ctx_get_option(&ctx, STRV_LIT("f"))) puts("first");
    if (ksh_ctx_get_option(&ctx, STRV_LIT("s"))) puts("second");
    if (ksh_ctx_get_option(&ctx, STRV_LIT("t"))) puts("third");
    if (ksh_ctx_get_option(&ctx, STRV_LIT("-"))) puts("dash");

    return 0;
}

static int print(KshContext ctx)
{
    KshErr err = ksh_ctx_init(&ctx, 2);
    if (err != KSH_ERR_OK) return err;

    StrView msg;
    if (!ksh_ctx_get_param(&ctx, STRV_LIT("m"), KSH_PARAM_TYPE_STR, &msg))
        return 1;

    int repeat;
    if (!ksh_ctx_get_param(&ctx, STRV_LIT("n"), KSH_PARAM_TYPE_INT, &repeat))
        repeat = 1;

    for (int i = 0; i < repeat; i++)
        printf(STRV_FMT"\n", STRV_ARG(msg));

    return 0;
}

int main()
{
    ksh_use_commands(
        { STRV_LIT("test"), test },
        { STRV_LIT("print"), print },
    );

    char buf[100];
    printf(">>> ");
    while (fgets(buf, sizeof(buf), stdin)) {
        if (strcmp(buf, "q\n") == 0) return 0;
        KshErr err = ksh_parse(strv_from_str(buf));
        if (err != 0) fprintf(stderr, "ERROR: %s\n", ksh_err_str(err));
        printf(">>> ");
    }

    return 0;
}

