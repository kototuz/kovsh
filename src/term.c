#include "kovsh.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void ksh_prompt_print(Prompt p)
{
    for (size_t i = 0; i < p.parts_len; i++) {
        ksh_termio_print(p.parts[i].text_prefs, p.parts[i].text);
    }
}

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
    handle_cmd_buf(term.commands);
    ksh_termio_init();

    while (true) {
#define MAX_LINE 100
        char line[MAX_LINE];

        ksh_prompt_print(term.prompt);
        ksh_termio_getline(MAX_LINE, line);

        if (strncmp(line, "quit\n", 4) == 0) break;
        Lexer lex = ksh_lexer_new((StrView){ .items = line, .len = strlen(line) });
        ksh_parse(&lex, &term);
    }

    ksh_termio_end();
}
