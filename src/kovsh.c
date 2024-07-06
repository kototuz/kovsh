#include "kovsh.h"

#include "lexer.c"
#include "strv.c"
#include "context.c"
#include "err.c"

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int dec(Lexer *args)
{
    StrView m;
    KshErr err = ksh_parse_args(args,
        KSH_PARAM(m, "message")
    );
    if (err != 0) return 1;

    printf("<["STRV_FMT"]>\n", STRV_ARG(m));
    
    return 0;
}

static int print(Lexer *args)
{
    StrView m;
    KshErr err = ksh_parse_args(args,
        KSH_HELP("prints your amazing messages"),
        KSH_PARAM(m, "message"),
        KSH_SUBCMD(dec, "print decorativly")
    );
    if (err != 0) return 1;

    printf(STRV_FMT"\n", STRV_ARG(m));

    return 0;
}

static int root(Lexer *args)
{
    KshErr err = ksh_parse_args(args,
        KSH_SUBCMD(print, "print message")
    );
    if (err != 0) return 1;

    return 0;
}

int main()
{
    char buf[100];
    printf(">>> ");
    while (fgets(buf, sizeof(buf), stdin)) {
        if (strcmp(buf, "q\n") == 0) return 0;
        KshErr err = ksh_parse_cmd(strv_from_str(buf), root);
        if (err != 0) fprintf(stderr, "ERROR: %s\n", ksh_err_str(err));
        printf(">>> ");
    }

    return 0;
}

