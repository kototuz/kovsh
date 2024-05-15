#include "kovsh.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int echo_fn(size_t argc, Arg argv[argc])
{
    printf("Message: "STRV_FMT"\n", STRV_ARG(argv[0].value.as_str));
    return 1;
}

int main(void)
{
    Terminal term = {
        .cmd_buf = (CommandBuf){
            .items = (Command[]){
                {
                    .name = STRV_LIT("echo"),
                    .fn = echo_fn,
                    .arg_defs = (ArgDef[]){
                        {
                            .name = STRV_LIT("msg"),
                            .type = ARG_VAL_TYPE_STR
                        }
                    },
                    .arg_defs_len = 1
                }
            },
            .len = 1
        },
        .prompt = ">> "
    };
    ksh_term_start(term);

    return 0;
}
