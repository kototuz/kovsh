#include "kovsh.h"
#include <ctype.h>

static bool lex_peek(Lexer *l);
static bool lex_next(Lexer *l);
static bool lex_next_if(Lexer *l, Token expected);
static bool lex_next_if_pred(Lexer *l, bool (*predicate)(Token));

static bool isstr(int s);
static bool isend(int s);
static bool isbound(int s);



static bool lex_peek(Lexer *l)
{
    if (l->is_peek) return true;

    Token result = { .items = &l->text.items[l->cursor] };

    if (isend(result.items[0])) return false;

    if (isspace(result.items[0])) {
        while (isspace(result.items[++result.len]));
        l->cursor += result.len;
        result.items = &result.items[result.len];
        result.len = 0;
    }

    if (isstr(result.items[0])) {
        int pairsymbol = result.items[0];
        while (result.items[++result.len] != pairsymbol)
            if (isend(result.items[result.len])) return false;
        result.len++;
    } else
        while (!isbound(result.items[++result.len]));

    l->is_peek = true;
    l->cur_tok = result;
    return true;
}

static bool lex_next(Lexer *l)
{
    if (l->is_peek) {
        l->cursor += l->cur_tok.len;
        l->is_peek = false;
        return true;
    }

    l->cur_tok = (Token){0};
    if (lex_peek(l)) {
        l->cursor += l->cur_tok.len;
        l->is_peek = false;
        return true;
    }

    return false;
}

static bool lex_next_if(Lexer *l, Token expected)
{
    return lex_peek(l)                   &&
           strv_eq(l->cur_tok, expected) &&
           lex_next(l);
}

static bool lex_next_if_pred(Lexer *l, bool (*predicate)(Token))
{
    return lex_peek(l)           &&
           predicate(l->cur_tok) &&
           lex_next(l);
}



static bool isstr(int s)
{
    return s == '"' || s == '\'';
}

static bool isend(int s)
{
    return s == '\0' || s == '\n';
}

static bool isbound(int s)
{
    return isspace(s) ||
           isstr(s)   ||
           isend(s);
}

#ifdef MAIN
int main()
{
    Lexer lex = { 
        .text = STRV_LIT("\t\rprint -m Hello -n 10 --hello-world -bro say_hi -m hello_world\n")
    };

    while (lex_next(&lex))
        printf(STRV_FMT"\n", STRV_ARG(lex.cur_tok));

    return 0;
}
#endif
