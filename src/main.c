#define COMMANDS_LEN 1
#include "kovsh.h"
#include <stdio.h>
#include <string.h>

int echo_fn(size_t argc, CommandArg argv[argc])
{
    printf("Message: ");
    strslice_print(argv[0].value.as_str, stdout);
    putchar('\n');
    return 1;
}

int main(void)
{
    Command echo = {
        .name = (StrSlice){ .items = "echo", .len = 4 },
        .desc = "Print message",
        .fn = echo_fn,
        .expected_args = (CommandArg[]){
            {
                .name = (StrSlice){ .items = "msg", .len = 3 },
                .usage = "Message that will been printing",
                .value.kind = COMMAND_ARG_VAL_KIND_STR,
                .value.as_str = (StrSlice){ .items = "Hello, World", .len = 12 }
            }
        },
        .expected_args_len = 1
    };
    ksh_command_print(echo);
    ksh_commands_add(echo);

    const char *text = "echo";
    StrSlice line = { .items = text, .len = strlen(text) };
    Lexer lexer = ksh_lexer_new(line);
    ksh_parse_lexer(&lexer);

    return 0;
}
