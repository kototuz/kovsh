#include "kovsh.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int echo_fn(size_t argc, Arg argv[argc])
{
    for (int i = 0; i < argv[1].value.as_int; i++) {
        printf("Message: "STRV_FMT"\n", STRV_ARG(argv[0].value.as_str));
    }
    return 1;
}

int main(void)
{
    assert(0 && "not yet implemented");
    return 0;
}
