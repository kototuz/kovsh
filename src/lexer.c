#include "kovsh.h"
#include <ctype.h>

static Lexer ksh_lexer_new(StrView sv);
static bool ksh_lexer_peek_token(Lexer *l, Token *t);
static bool ksh_lexer_next_token(Lexer *l, Token *t);
static KshErr ksh_lexer_expect_next_token(Lexer *l, Token expected);

static bool isend(int s);
static bool isbound(int s);

Lexer ksh_lexer_new(StrView strv)
{
    return (Lexer){.text = strv};
}

bool ksh_lexer_peek_token(Lexer *l, Token *t)
{
    if (l->buf.items) {
        *t = l->buf;
        return true;
    }

    Token result = { .items = &l->text.items[l->cursor] };

    if (result.items[0] == '\0') return false;

    if (isspace(result.items[0])) {
        while (isspace(result.items[++result.len]));
        l->cursor += result.len;
        result.items = &result.items[result.len];
        result.len = 0;
    }

    if (result.items[0] == '"' || result.items[0] == '\'') {
        int pairsymbol = result.items[0];
        while (result.items[++result.len] != pairsymbol)
            if (isend(result.items[result.len])) return false;
        result.len++;
    } else if (ispunct(result.items[0])) {
        result.len++;
    } else
        while (!isbound(result.items[++result.len]));

    l->buf = result;
    *t = result;
    return true;
}

bool ksh_lexer_next_token(Lexer *l, Token *t)
{
    if (ksh_lexer_peek_token(l, t)) {
        l->cursor += t->len;
        l->buf = (Token){0};
        return true;
    }

    return false;
}

KshErr ksh_lexer_expect_next_token(Lexer *l, Token expected)
{
    Token tok;
    if (!ksh_lexer_next_token(l, &tok) ||
        !strv_eq(tok, expected)) return KSH_ERR_TOKEN_EXPECTED;

    return KSH_ERR_OK;
}



static bool isend(int s)
{
    return s == '\0' || s == '\n';
}

static bool isbound(int s)
{
    return isspace(s) ||
           ispunct(s) || // TODO: make a separate function
           isend(s);
}

int test()
{
    Lexer lex = ksh_lexer_new(strv_from_str("Neque porro quisquam est qui dolorem ipsum quia dolor\t\rsit amet, consectetur, adipisci velit, msg='Hello, World'"));
    
    assert(ksh_lexer_expect_next_token(&lex, strv_from_str("Neque")) == KSH_ERR_OK);
    assert(ksh_lexer_expect_next_token(&lex, strv_from_str("porro")) == KSH_ERR_OK);
    assert(ksh_lexer_expect_next_token(&lex, strv_from_str("moon")) == KSH_ERR_TOKEN_EXPECTED);

    return 0;
}
