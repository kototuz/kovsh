#include "../kovsh.h"

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int flot(KshParser *p)
{
    float v;
    p->params = KSH_PARAMS(KSH_PARAM(v, "float"));
    ksh_parse_args(p);

    printf("%f\n", v);

    return 0;
}

static int str(KshParser *p)
{
    StrView v;
    p->params = KSH_PARAMS(KSH_PARAM(v, "str"));
    ksh_parse_args(p);

    printf(STRV_FMT"\n", STRV_ARG(v));

    return 0;
}

static int print(KshParser *parser)
{
    parser->flags = KSH_FLAGS(KSH_HELP("printer powered by KOVSH"));
    parser->subcmds = KSH_SUBCMDS(
        KSH_SUBCMD(flot, "print float"),
        KSH_SUBCMD(str, "print str")
    );

    ksh_parse_args(parser);

    return 0;
}

static int root(Lexer *l)
{
    StrView m;
    int n;
    bool yes;
    ksh_parse_args(l, KSH_ARGS(
        KSH_PARAMS(
            KSH_PARAM(m, "msg"),
            KSH_PARAM(n, "count")
        ),
        KSH_FLAGS(
            KSH_FLAG(yes, "yes?"),
            KSH_HELP("maybe what you want")
        ),
        KSH_SUBCMDS(
            KSH_SUBCMD(print, "printer"),
        )
    ));

    ksh_params = (KshParam[]){
        KSH_PARAM(s, "string"),
        KSH_PARAM()
    };

    ksh_parse_args(l,
        KSH_PARAMS(KSH_PARAM(m, "message")),
        KSH_OPT_PARAMS(KSH_STORE(m, ))
        KSH_FLAGS(KSH_FLAG(yes, "yes?")),
        KSH_SUBCMDS(KSH_SUBCMD(print, "printer")),
        KSH_HELP("privet men"),
        KSH_VERSION("0.0.1")
    );

    ksh_parse_args(
        KSH_PARAMS(
            KSH_PARAM(m, "message"),
            KSH_PARAM(n, "count"),
        )
    );

    ksh_parse_args(parser);
    return 0;
}

int main(void)
{
    char buf[100];
    KshParser parser = { .root = root };

    printf(">>> ");
    while (fgets(buf, sizeof(buf), stdin)) {
        if (strcmp(buf, "q\n") == 0) return 0;
        ksh_parse_strv(strv_from_str(buf), root);
        printf(">>> ");
    }

    return 0;
}
