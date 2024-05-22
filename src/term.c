#include "kovsh.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define FG_COLOR_BEG 29
#define BG_COLOR_BEG 39

#define MODE_COUNT 5
#define BOLD_MODE 1
#define ITALIC_MODE 3
#define UNDERSCORED_MODE 4
#define BLINK_MODE 5
#define STRIKETHROUGH_MODE 9

static const int mode_seq[] = {
    BOLD_MODE,
    ITALIC_MODE,
    UNDERSCORED_MODE,
    BLINK_MODE,
    STRIKETHROUGH_MODE,
};

void ksh_prompt_print(Prompt p)
{
    for (size_t i = 0; i < p.parts_len; i++) {
        union {
            TermTextMode as_mode;
            bool as_arr[MODE_COUNT];
        } mode = { .as_mode = p.parts[i].text_prefs.mode };

        printf("\033[0");
        for (size_t i = 0; i < MODE_COUNT; i++) {
            if (mode.as_arr[i]) {
                printf(";%d", mode_seq[i]);
            }
        }

        TermColor fg = p.parts[i].text_prefs.fg_color;
        TermColor bg = p.parts[i].text_prefs.bg_color;
        if (fg) printf(";%d", FG_COLOR_BEG + fg); 
        if (bg) printf(";%d", BG_COLOR_BEG + bg);

        printf("m%s\033[0m", p.parts[i].text);
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
    handle_cmd_buf(term.cmd_buf);
    while (true) {
        char *line = {0};
        size_t len = 0;

        ksh_prompt_print(term.prompt);
        if ((getline(&line, &len, stdin)) != -1) {
            if (strcmp(line, "quit\n") == 0) return;
            Lexer lex = ksh_lexer_new((StrView){ .items = line, .len = len });
            ksh_parse(&(Parser){ .info.cmds = term.cmd_buf, .lex = lex });
        }
    }
}
