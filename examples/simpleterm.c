#include "../kovsh.h"

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int flot(KshParser *p)
{
    float v;
    p->params = KSH_PARAMS(KSH_STORE(v, "float"));
    if (!ksh_parse(p)) return 0;

    printf("%f\n", v);

    return 0;
}

static int str(KshParser *p)
{
    StrView v;
    p->params = KSH_PARAMS(KSH_STORE(v, "str"));
    if (!ksh_parse(p)) return 0;

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

    return ksh_parse(parser);
}

static int root(KshParser *parser)
{
    parser->flags = KSH_FLAGS(KSH_HELP("a simple terminal powered by KOVSH"));
    parser->subcmds = KSH_SUBCMDS(KSH_SUBCMD(print, "a printer"));

    if (!ksh_parse(parser)) {
        if (parser->err[0]) {
            fprintf(stderr, "error: %s\n", parser->err);
            return 1;
        }
        return 0;
    }

    return 0;
}

int main(void)
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
