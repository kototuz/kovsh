#include "../kovsh.h"

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int print(KshArgParser *parser)
{
    StrView m;
    int n = 1;
    if (!ksh_parser_parse_args(parser,
        KSH_HELP("displays your amazing messages"),
        KSH_PARAM(m, "message"),
        KSH_PARAM(n, "count")
    )) return 0;

    for (int i = 0; i < n; i++)
        printf(STRV_FMT"\n", STRV_ARG(m));

    return 0;
}

static int root(KshArgParser *parser)
{
    if (!ksh_parser_parse_args(parser,
        KSH_HELP("a simple terminal"),
        KSH_SUBCMD(print, "displays your amazing messages"),
    ) && parser->err[0]) {
        printf("error: %s", parser->err);
        return 1;
    }

    return 0;
}

int main()
{
    char buf[100];
    KshArgParser parser = {0};

    printf(">>> ");
    while (fgets(buf, sizeof(buf), stdin)) {
        if (strcmp(buf, "q\n") == 0) return 0;
        ksh_parser_parse_cmd(&parser, root, strv_from_str(buf));
        printf(">>> ");
    }

    return 0;
}
