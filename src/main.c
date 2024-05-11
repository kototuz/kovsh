#define COMMANDS_LEN 1
#include "kovsh.h"
#include <stdio.h>
#include <string.h>

int echo_fn(size_t argc, CommandArg argv[argc])
{
    printf("Message: "STRV_FMT, STRV_ARG(argv[0].value.as_str));
    return 1;
}

int main(void)
{
    Command echo = {
        .name = STRV_LIT("echo"),
        .desc = "Print message",
        .fn = echo_fn,
        .expected_args = (CommandArg[]){
            {
                .name = STRV_LIT("msg"),
                .usage = "Message that will been printing",
                .value.kind = COMMAND_ARG_VAL_KIND_STR,
                .value.as_str = STRV_LIT("All hail Britania!!!")
            }
        },
        .expected_args_len = 1
    };
    ksh_command_print(echo);
    ksh_commands_add(echo);

    StrView line = STRV_LIT("echo msg=");
    Lexer lexer = ksh_lexer_new(line);

    if (ksh_parse_lexer(&lexer) != KSH_ERR_OK) {
        return 1;
    }

    return 0;
}
