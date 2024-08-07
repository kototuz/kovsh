# Kovsh
## Desription
Kovsh is a command parser written in C

## Get started
``` c
#include <stdio.h>
#include "kovsh.h"

int root(KshParser *p)
{
    StrView m;
    ksh_parse_args(p, &(KshArgs){  // parse the specified arguments
        .params = KSH_PARAMS(KSH_PARAM(m, "message")),
        .help = "prints your messages"
    });

    // and do whatever you want

    printf(STRV_FMT"\n", STRV_ARG(m));

    return 0;
}

int main(int argc, char **argv)
{
    KshParser parser;                         // declare a new parser
    ksh_init_from_cargs(&parser, argc, argv); // init the parser
    ksh_parse(&parser, root);                 // parse 
}
```
How it looks:
``` console
$ ./test +m hello
```

You can also parse just a string:
``` c
    int main(void)
    {
        KshParser parser;
        ksh_init_from_cstr(&parser, "+m hello");
        ksh_parse(&parser, root); // root is the same as in the previous example
    }
```

To get more info see examples :)
