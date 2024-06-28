#include "kovsh.h"
#include <ctype.h>

static bool lex_peek_tok(Lexer *l, Token *t);
static bool lex_next_tok(Lexer *l, Token *t);
static bool lex_next_tok_if_tok(Lexer *l, Token expected);
static bool lex_next_tok_if_pred(Lexer *l, bool (*predicate)(Token));

static bool isstr(int s);
static bool isend(int s);
static bool isbound(int s);



static bool lex_peek_tok(Lexer *l, Token *t)
{
    if (l->buf.items) {
        *t = l->buf;
        return true;
    }

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

    l->buf = result;
    *t = result;
    return true;
}

static bool lex_next_tok(Lexer *l, Token *t)
{
    if (lex_peek_tok(l, t)) {
        l->cursor += t->len;
        l->buf = (Token){0};
        return true;
    }

    return false;
}

static bool lex_next_tok_if_tok(Lexer *l, Token expected)
{
    Token tok;
    return lex_next_tok(l, &tok) &&
           strv_eq(expected, tok);
}

static bool lex_next_tok_if_pred(Lexer *l, bool (*predicate)(Token))
{
    Token tok;
    return lex_next_tok(l, &tok) &&
           predicate(tok);
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

int test()
{
    Lexer lex = { 
        .text = STRV_LIT("Neque porro quisquam est qui dolorem ipsum quia dolor\t\rsit amet, consectetur, adipisci velit, msg='Hello, World")
    };
    
    assert(lex_next_tok_if_tok(&lex, strv_from_str("Neque")));
    assert(lex_next_tok_if_tok(&lex, strv_from_str("porro")));
    assert(lex_next_tok_if_tok(&lex, strv_from_str("moon")));

    return 0;
}
