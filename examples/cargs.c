#include <stdio.h>
#include "../kovsh.h"

static int stuf(KshParser *p) {
    int num;
    ksh_parse_args(p, &(KshArgs){
        .params = KSH_PARAMS(KSH_PARAM(num, "a number"))
    });

    if (num < 10) return 1;

    puts("Oh your the best");
    return 0;
}

static int main2(KshParser *p)
{
    char m[10];
    ksh_parse_args(p, &(KshArgs){
        .opt_params = KSH_PARAMS(KSH_PARAM(m, "message")),
        .help = "a simple print command powered by KOVSH",
        .subcmds = KSH_SUBCMDS(KSH_SUBCMD(stuf, "stuf"))
    });

    printf("Your message is: %s\n", m);
    return 0;
}

int main(int argc, char **argv)
{
    KshParser p;
    ksh_init_from_cargs(&p, argc, argv);
    ksh_parse(&p, main2);

    if (p.cmd_exit_code != 0) {
        printf("The cmd returned an error: %d\n", p.cmd_exit_code);
        return 1;
    }

    return 0;
}
