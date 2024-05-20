#include "kovsh.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void handle_cmd_buf(CommandBuf buf)
{
    for (size_t i = 0; i < buf.len; i++) {
        Command cmd = buf.items[i];

        assert(cmd.name.items);
        assert(cmd.fn);

        for (size_t y = 0; y < cmd.arg_defs_len; y++) {
            assert(cmd.arg_defs[y].name.items);
            if (cmd.arg_defs[y].usage == 0) {
                cmd.arg_defs[y].usage = "None";
            }
        }

        if (cmd.desc == 0) {
            buf.items[i].desc = "None";
        }
    }
}

void ksh_term_start(Terminal term)
{
    handle_cmd_buf(term.cmd_buf);
    while (true) {
        char *line = {0};
        size_t len = 0;

        printf(term.prompt);
        if ((getline(&line, &len, stdin)) != -1) {
            if (strcmp(line, "quit\n") == 0) return;
            Lexer lex = ksh_lexer_new((StrView){ .items = line, .len = len });
            ksh_parse(&(Parser){ .info.cmds = term.cmd_buf, .lex = lex });
        }
    }
}
