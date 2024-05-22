#include "kovsh.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

int echo_fn(size_t argc, Arg argv[argc])
{
    for (int i = 0; i < argv[1].value.as_int; i++) {
        printf("Message: "STRV_FMT"\n", STRV_ARG(argv[0].value.as_str));
    }

    return 1;
}

int main(void)
{
    char login[10];
    getlogin_r(login, 10);

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
                        },
                        {
                            .name = STRV_LIT("rep"),
                            .type = ARG_VAL_TYPE_INT,
                            .has_default = true,
                            .default_val.as_int = 1,
                        }
                    },
                    .arg_defs_len = 2
                }
            },
            .len = 1
        },
        .prompt.parts_len = 2,
        .prompt.parts = (PromptPart[]){
            {
                .text = login,
                .text_prefs.mode.blink = true,
                .text_prefs.fg_color = TERM_COLOR_GREEN
            },
            {
                .text = "> ",
                .text_prefs.mode.blink = true,
                .text_prefs.fg_color = TERM_COLOR_GREEN
            }
        }
    };

    ksh_term_start(term);

    return 0;
}
