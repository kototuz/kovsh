#include <stdio.h>
#include "../kovsh.h"

static int main2(KshParser *p)
{
    StrView m = STRV_LIT("hello");
    StrView c = STRV_LIT("\033[31m");
    p->params = KSH_PARAMS(
        KSH_STORE(m, "message"),
        KSH_STORE(c, "color")
    );
    ksh_parse_args(p);

    if (strv_eq(c, STRV_LIT("green"))) {
        c = STRV_LIT("\033[32m");
    } else if (strv_eq(c, STRV_LIT("red"))) {
        c = STRV_LIT("\033[31m");
    }

    printf(STRV_FMT""STRV_FMT"\n",
           STRV_ARG(c),
           STRV_ARG(m));

    return 0;
}

int main(int argc, char **argv)
{
    KshParser p = {0};
    ksh_init_from_cargs(&p, argc, argv);
    ksh_parse_cmd(&p, main2);
    return 0;
}
