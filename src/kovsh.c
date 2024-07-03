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
    Lexer lex = { .text = sv };

    if (!lex_next(&lex)) return KSH_ERR_OK;

    command = get_cmd(lex.cur_tok);
    if (!command) return KSH_ERR_COMMAND_NOT_FOUND;

    KshErr err;
    command->fn((KshContext){ .lex = lex }, &err);
    return err;
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


static KSH_CMD(print)
{
    StrView m;
    int n = 1;
    KSH_INIT(
        "Prints messages",
        KSH_PARAM(m, "Message"),
        KSH_PARAM(n, "Count"),
    );

    for (int i = 0; i < n; i++)
        printf(STRV_FMT"\n", STRV_ARG(m));

    return 0;
}

int main()
{
    ksh_use_commands(
        { STRV_LIT("print"), print }
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

