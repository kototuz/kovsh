
typedef bool (*ParseFn)(StrView in, void *res);



static bool parse_str(StrView in, StrView *res);
static bool parse_int(StrView in, int *res);

static bool full_name_predicate(Token tok);
static bool short_name_predicate(Token tok);
static bool multiopt_predicate(Token tok);
static bool notarg_predicate(Token tok);

static KshArgDef *get_arg_def(size_t count, KshArgDef items[count], StrView name);



static const ParseFn parsemap[] = {
    [KSH_ARG_KIND_PARAM_STR] = (ParseFn) parse_str,
    [KSH_ARG_KIND_PARAM_INT] = (ParseFn) parse_int
};



KshErr ksh_ctx_init_(KshContext *ctx, size_t size, KshArgDef arg_def_buf[size])
{
    Lexer *lex = &ctx->lex;
    KshArgDef *arg_def;

    while (lex_peek(lex)) {
        if (lex_next_if_pred(lex, full_name_predicate)) {
            arg_def = get_arg_def(size, arg_def_buf, (StrView){
                .items = &lex->cur_tok.items[2],
                .len = lex->cur_tok.len-2
            });
        } else if (lex_next_if_pred(lex, short_name_predicate)) {
            arg_def = get_arg_def(size, arg_def_buf, (StrView){
                .items = &lex->cur_tok.items[1],
                .len = 1,
            });
        } else if (lex_next_if_pred(lex, multiopt_predicate)) {
            for (size_t i = 1; i < lex->cur_tok.len; i++) {
                arg_def = get_arg_def(size, arg_def_buf, (StrView){
                    .items = &lex->cur_tok.items[i],
                    .len = 1
                });
                if (!arg_def) return KSH_ERR_ARG_NOT_FOUND;

                assert(arg_def->kind == KSH_ARG_KIND_OPT);

                *((bool*)arg_def->dest) = true;
            }
            continue;
        } else return KSH_ERR_TOKEN_EXPECTED;

        if (!arg_def) return KSH_ERR_ARG_NOT_FOUND;

        if (IS_PARAM(arg_def->kind)) {
            if (!lex_next_if_pred(lex, notarg_predicate))
                return KSH_ERR_VALUE_EXPECTED;
            if (!parsemap[arg_def->kind](lex->cur_tok, arg_def->dest))
                return KSH_ERR_PARSING_FAILED;
        } else {
            *((bool*)arg_def->dest) = true;
        }
    }

    return KSH_ERR_OK;
}



static KshArgDef *get_arg_def(size_t count, KshArgDef items[count], StrView name)
{
    for (size_t i = 0; i < count; i++) {
        if (strv_eq(name, items[i].name)) {
            return &items[i];
        }
    }

    return NULL;
}

static bool parse_str(StrView in, StrView *res)
{
    if (in.items[0] == '"' || in.items[0] == '\'') {
        res->items = &in.items[1];
        res->len = in.len-2;
    } else *res = in;

    return true;
}

static bool parse_int(StrView in, int *res)
{
    int result = 0;

    for (size_t i = 0; i < in.len; i++) {
        if (!isdigit(in.items[i])) return false;
        result = result*10 + in.items[i]-'0';
    }

    *res = result;
    return true;
}

static bool full_name_predicate(Token tok)
{
    return tok.len > 3         &&
           tok.items[0] == '-' &&
           tok.items[1] == '-';
}

static bool short_name_predicate(Token tok)
{
    return tok.len == 2 && tok.items[0] == '-';
}

static bool multiopt_predicate(Token tok)
{
    return tok.len > 2         &&
           tok.items[0] == '-' &&
           tok.items[1] != '-';
}

static bool notarg_predicate(Token tok)
{
    return tok.items[0] != '-';
}
