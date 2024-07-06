
typedef bool (*ParseFn)(StrView in, void *res);



static const char *arg_name_prefix(StrView name);

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



bool ksh_parse_args(KshContext ctx, KshErr *parsing_err)
{
    KshArgDef *arg_def;
    StrView arg_name;
    while (lex_peek(&ctx.lex)) {
        if (lex_next_if_pred(&ctx.lex, full_name_predicate)) {
            arg_name = (StrView){
                .items = &ctx.lex.cur_tok.items[2],
                .len = ctx.lex.cur_tok.len-2
            };
        } else if (lex_next_if_pred(&ctx.lex, short_name_predicate)) {
            arg_name = (StrView){
                .items = &ctx.lex.cur_tok.items[1],
                .len = 1
            };
        } else if (lex_next_if_pred(&ctx.lex, multiopt_predicate)) {
            for (size_t i = 1; i < ctx.lex.cur_tok.len; i++) {
                arg_def = get_arg_def(ctx.arg_defs_count, ctx.arg_defs, (StrView){
                    .items = &ctx.lex.cur_tok.items[i],
                    .len = 1
                });
                if (!arg_def) {
                    *parsing_err = KSH_ERR_ARG_NOT_FOUND;
                    return false;
                }

                assert(arg_def->kind == KSH_ARG_KIND_OPT);

                *((bool*)arg_def->dest) = true;
            }
            continue;
        } else {
            *parsing_err = KSH_ERR_TOKEN_EXPECTED;
            return false;
        }

        arg_def = get_arg_def(ctx.arg_defs_count, ctx.arg_defs, arg_name);
        if (!arg_def) {
            *parsing_err = KSH_ERR_ARG_NOT_FOUND;
            return false;
        }

        if (IS_PARAM(arg_def->kind)) {
            if (!lex_next_if_pred(&ctx.lex, notarg_predicate)) {
                *parsing_err = KSH_ERR_VALUE_EXPECTED;
                return false;
            }
            if (!parsemap[arg_def->kind](ctx.lex.cur_tok, arg_def->dest)) {
                *parsing_err = KSH_ERR_PARSING_FAILED;
                return false;
            }
        } else if (arg_def->kind == KSH_ARG_KIND_OPT) {
            *((bool*)arg_def->dest) = true;
        } else if (arg_def->kind == KSH_ARG_KIND_HELP) {
            printf("[descr]: %s\n", (char *)arg_def->dest);
            for (size_t i = 0; i < ctx.arg_defs_count; i++) {
                printf("  %2s%-15s%s\n",
                       arg_name_prefix(ctx.arg_defs[i].name),
                       ctx.arg_defs[i].name.items,
                       ctx.arg_defs[i].usage);
            }
        }
    }

    *parsing_err = KSH_ERR_OK;
    return true;
}



static const char *arg_name_prefix(StrView name)
{
    return name.len > 1 ? "--" : "-";
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
