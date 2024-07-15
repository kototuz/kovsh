#include <stdio.h>
#include "../kovsh.h"

int main(int argc, char **argv)
{
    KshParser p = {0};

    StrView msg = STRV_LIT("hello world");
    KSH_PARAMS(&p, KSH_STORE(msg, "message"));
    if (!ksh_parse_cargs(&p, argc, argv)) return 1;

    printf(STRV_FMT"\n", STRV_ARG(msg));

    return 0;
}
