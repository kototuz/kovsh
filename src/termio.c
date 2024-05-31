#include "kovsh.h"

#if TERMIO == TERMIO_NCURSES

#include <ncurses.h>

static const int mode_map[TERM_TEXT_MODE_COUNT] = {
    A_BOLD,
    A_ITALIC,
    A_UNDERLINE,
    A_BLINK,
    A_REVERSE
};


void ksh_termio_init(void)
{
    initscr();
}

void ksh_termio_getline(size_t size, char buf[size])
{
    getstr(buf);
}

void ksh_termio_print(TermTextPrefs prefs, const char *txt, ...)
{
    static int color_palette[TERM_COLOR_COUNT][TERM_COLOR_COUNT] = {0};
    static int color_pair_idx = 1;

    va_list args;
    va_start(args, txt);

    union {
        TermTextMode as_mode;
        bool as_arr[5]; 
    } mode = { .as_mode = prefs.mode };

    for (size_t i = 0; i < 5; i++) {
        if (mode.as_arr[i]) {
            attron(mode_map[i]);
        }
    }

    if (has_colors() == TRUE) {
        start_color();

        int fg = prefs.fg_color > 0 ? prefs.fg_color-1 : COLOR_WHITE;
        int bg = prefs.bg_color > 0 ? prefs.bg_color-1 : COLOR_BLACK;
        int idx = color_palette[fg][bg];
        if (idx == 0) {
            init_pair(color_pair_idx, fg, bg);
            attron(COLOR_PAIR(color_pair_idx));
            color_palette[fg][bg] = color_pair_idx++;
        } else {
            attron(COLOR_PAIR(idx));
        }
    }

    vw_printw(stdscr, txt, args);
    refresh();

    standend();

    va_end(args);
}

void ksh_termio_end(void)
{
    endwin();
}
#elif TERMIO == TERMIO_DEFAULT

void ksh_termio_init(void) {}
void ksh_termio_end(void) {}

void
ksh_termio_print(TermTextPrefs prefs, const char *fmt, ...)
{
#warning "Text preferences are not available in TERMIO_DEFAULT now"
    (void) prefs;
    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);

    va_end(args);
}

void
ksh_termio_getline(size_t size, char buf[size])
{
    fgets(buf, size, stdin);
}

#endif // TERMIO

