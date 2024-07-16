#include <stdio.h>
#include "../kovsh.h"

static int main2(KshParser *p)
{
    StrView m = STRV_LIT("hello");
    p->params = KSH_PARAMS(KSH_STORE(m, "message"));
    ksh_parse_args(p);
    printf(STRV_FMT"\n", STRV_ARG(m));
    return 0;
}

int main(int argc, char **argv)
{
    KshParser p = {0};
    ksh_init_from_cargs(&p, argc, argv);
    ksh_parse_cmd(&p, main2);
    return 0;
}
