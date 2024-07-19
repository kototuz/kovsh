#include <stdio.h>
#include "../kovsh.h"

static int main2(KshParser *p)
{
    StrView m;
    ksh_parse_args(p, &(KshArgs){
        .params = KSH_PARAMS(KSH_PARAM(m, "message")),
        .help = "a simple terminal written in C and powered by KOVSH",
    });

    printf(STRV_FMT"\n", STRV_ARG(m));
    return 0;
}

int main(int argc, char **argv)
{
    ksh_parse_cargs(argc, argv, main2);
    return 0;
}
