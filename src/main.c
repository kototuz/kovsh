#include <stdio.h>
#include <string.h>

#include "cmd.h"

int main(void)
{
    RuleArray arg_rules = {
        .rules = (Rule[]){ rule_simple(TOKEN_SYMBOL) },
        .len = 1
    };

    char *line = "echo asdf";
    Lexer lexer = lexer_new(line, strlen(line));

    TokenSeq token_seq = lexer_tokenize(&lexer);

    if (syntax_is_valid(&token_seq, &arg_rules)) {
        puts("Ok");
    } else {
        puts("Err!");
    }


    return 0;
}
