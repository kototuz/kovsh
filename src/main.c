#define COMMANDS_LEN 1
#include "kovsh.h"
#include <stdio.h>
#include <string.h>

int echo_fn(size_t argc, Arg argv[argc])
{
    for (int i = 0; i < argv[1].value.as_int; i++) {
        printf("Message: "STRV_FMT"\n", STRV_ARG(argv[0].value.as_str));
    }
    return 1;
}

int main(void)
{
    Command echo = {
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
                .default_val.as_int = 1
            },
        },
        .arg_defs_len = 2
    };

    Command *cmd = ksh_cmds_add(echo);
    ksh_cmd_print(*cmd);

    StrView line = STRV_LIT("echo msg=Hello");
    Lexer lexer = ksh_lexer_new(line);

    KshErr err = ksh_parse_lexer(&lexer);
    if (err != 0) return err;

    return 0;
}
