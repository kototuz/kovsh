#include "../cmd.h"

void expr_parse(Parser *p, TokenCursor *token_cursor)
{
    assert(token_cursor->count != 0);

    while (!token_cursor_is_end(token_cursor)) {
        TokenNode node = token_cursor->current;

        ExprMakeFn make = lookup[node->val.type];
        make(node->val);
    }
}

Expr *expr_build_next(TokenCursor *tc)
{
    TokenType tt = tc->current->val.type;
    switch (tt) {
        case TOKEN_SYMBOL:
            return (Expr) {
                .value = tc->current->val->data;
            };
            break;
        case TOKEN_EQ:
            return (Expr)
    }
}
