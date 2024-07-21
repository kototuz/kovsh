#include "../kovsh.h"

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int print(KshParser *p)
{
    StrView m;
    ksh_parse_args(p, &(KshArgs){
        .params = KSH_PARAMS(KSH_PARAM(m, "message"))
    });

    printf(STRV_FMT"\n", STRV_ARG(m));

    return 0;
}

static int root(KshParser *p)
{
    ksh_parse_args(p, &(KshArgs){
        .help = "a simple terminal powered by KOVSH",
        .subcmds = KSH_SUBCMDS(KSH_SUBCMD(print, "print"))
    });

    return 0;
}

int main(void)
{
    char buf[100];
    KshParser parser;

    printf(">>> ");
    while (fgets(buf, sizeof(buf), stdin)) {
        if (strcmp(buf, "q\n") == 0) return 0;
        ksh_init_from_cstr(&parser, buf);
        ksh_parse(&parser, root);
        printf(">>> ");
    }

    return 0;
}
