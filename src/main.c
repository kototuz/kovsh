#include "kovsh.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

int echo_fn(size_t argc, Arg argv[argc])
{
    for (int i = 0; i < argv[1].value.as_int; i++) {
        ksh_termio_print((TermTextPrefs){0}, "Message: "STRV_FMT"\n", STRV_ARG(argv[0].value.as_str));
    }

    return 1;
}

int main(void)
{
    char login[10];
    getlogin_r(login, 10);

    Terminal term = {
        .prompt.parts_len = 2,
        .prompt.parts = (PromptPart[]){
            {
                .text = login,
                .text_prefs.mode.bold = true,
                .text_prefs.fg_color = TERM_COLOR_GREEN
            },
            {
                .text = "$ ",
                .text_prefs.fg_color = TERM_COLOR_GREEN
            }
        }
    };

    ksh_term_start(term);

    return 0;
}
