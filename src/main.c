#define COMMANDS_LEN 1
#include "kovsh.h"
#include <stdio.h>
#include <string.h>

int echo_fn(size_t argc, CommandArg argv[argc])
{
    for (int i = 0; i < argv[1].value.as_int; i++) {
        printf("Message: "STRV_FMT"\n", STRV_ARG(argv[0].value.as_str));
    }
    return 1;
}

int main(void)
{
    Command echo = ksh_command_new((CommandOpt){
        .name = "echo",
        .fn = echo_fn,
        .argv = (CommandArg[]){
            ksh_commandarg_new((CommandArgOpt){
                .name = "msg",
                .value.kind = COMMAND_ARG_VAL_KIND_STR,
                .value.as_str = STRV_LIT("All Hail Britania!!!")
            }),
            ksh_commandarg_new((CommandArgOpt){
                .name = "rep",
                .value.kind = COMMAND_ARG_VAL_KIND_INT,
                .value.as_int = 1,
            })
        },
        .argc = 2
    });

    ksh_command_print(echo);
    ksh_commands_add(echo);

    StrView line = STRV_LIT("echo rep=100");
    Lexer lexer = ksh_lexer_new(line);

    if (ksh_parse_lexer(&lexer) != KSH_ERR_OK) {
        return 1;
    }

    return 0;
}
