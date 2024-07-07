#include "kovsh.h"

#include "lexer.c"
#include "strv.c"
#include "context.c"
#include "err.c"

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int dec(KshArgParser *parser)
{
    StrView m;
    if (!ksh_parser_parse_args(parser,
        KSH_PARAM(m, "message")
    )) return 1;

    printf("<["STRV_FMT"]>\n", STRV_ARG(m));
    
    return 0;
}

static int print(KshArgParser *parser)
{
    StrView m;
    if (!ksh_parser_parse_args(parser,
        KSH_HELP("prints your amazing messages"),
        KSH_PARAM(m, "message"),
        KSH_SUBCMD(dec, "print decorativly")
    )) return 1;

    printf(STRV_FMT"\n", STRV_ARG(m));

    return 0;
}

static int root(KshArgParser *parser)
{
    if (!ksh_parser_parse_args(parser,
        KSH_SUBCMD(print, "print message")
    ) && parser->err_code != KSH_ERR_EARLY_EXIT) {
        printf("[err]: %s\n", parser->err_msg);
    }


    return 0;
}

int main()
{
    char buf[100];
    printf(">>> ");
    while (fgets(buf, sizeof(buf), stdin)) {
        if (strcmp(buf, "q\n") == 0) return 0;
        ksh_parse_cmd(strv_from_str(buf), root);
        printf(">>> ");
    }

    return 0;
}

