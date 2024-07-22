#include <stdio.h>
#include "../kovsh.h"

static int main2(KshParser *p)
{
    char m[10];
    ksh_parse_args(p, &(KshArgs){
        .opt_params = KSH_PARAMS(KSH_PARAM(m, "message")),
        .help = "a simple print command powered by KOVSH",
    });

    printf("Your message is: %s\n", m);
    return 0;
}

int main(int argc, char **argv)
{
    KshParser p;
    ksh_init_from_cargs(&p, argc, argv);
    ksh_parse(&p, main2);

    return 0;
}
