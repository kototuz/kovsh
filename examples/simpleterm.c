#include "../kovsh.h"

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int print(KshParser *parser)
{
    bool dec = false;

    StrView m;
    int n = 1;
    KSH_PARAMS(parser,
        KSH_PARAM(m, "message"),
        KSH_PARAM(n, "count")
    );
    KSH_FLAGS(parser,
        KSH_FLAG(dec, "decorativly")
    );
    if (!ksh_parse(parser)) return 0;

    if (dec) {
        for (; n > 0; n--) 
            printf("<<"STRV_FMT">>\n", STRV_ARG(m));
        return 0;
    }

    for (; n > 0; n--) 
        printf(STRV_FMT"\n", STRV_ARG(m));

    return 0;
}

static int root(KshParser *parser)
{
    KSH_SUBCMDS(parser,
        KSH_SUBCMD(print, "prints messages")
    );

    if (!ksh_parse(parser) && parser->err[0]) {
        fprintf(stderr, "error: %s\n", parser->err);
        return 1;
    }

    return 0;
}

int main()
{
    char buf[100];
    KshParser parser = { .root = root };

    printf(">>> ");
    while (fgets(buf, sizeof(buf), stdin)) {
        if (strcmp(buf, "q\n") == 0) return 0;
        ksh_parse_cmd(&parser, strv_from_str(buf));
        printf(">>> ");
    }

    return 0;
}
