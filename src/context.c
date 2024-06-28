
typedef bool (*ParseFn)(StrView in, void *res);


static KshArg *ksh_ctx_get_arg(KshContext *ctx, StrView name);

static bool parse_str(StrView in, StrView *res);
static bool parse_int(StrView in, int *res);

static bool arg_predicate(Token tok);



static const ParseFn parsemap[] = {
    [KSH_PARAM_TYPE_STR] = (ParseFn) parse_str,
    [KSH_PARAM_TYPE_INT] = (ParseFn) parse_int
};



KshErr ksh_ctx_init_(KshContext *ctx, size_t size, KshArg arg_buf[size])
{
    KshArg result;
    KshErr err;
    size_t count = 0;
    Lexer *lex = ctx->lex;

    while (count < size && ksh_lexer_peek_token(lex, &result.name)) {
        err = ksh_lexer_expect_next_token_pred(lex, arg_predicate);
        if (err != KSH_ERR_OK) return err;

        result.name.items = &result.name.items[2];
        result.name.len -= 2;

        if (
            !ksh_lexer_next_token(lex, &result.value) ||
            result.value.items[0] == '-' // TODO: make a separate function
        ) result.value = (StrView){0};

        arg_buf[count++] = result;
    }

    ctx->args = arg_buf;
    ctx->args_count = count;
    return KSH_ERR_OK;
}

bool ksh_ctx_get_param(KshContext *ctx,
                       StrView name,
                       KshParamType param_type,
                       void *res)
{
    KshArg *arg = ksh_ctx_get_arg(ctx, name);
    if (!arg) return false;

    return parsemap[param_type](arg->value, res);
}

bool ksh_ctx_get_option(KshContext *ctx, StrView name)
{
    KshArg *arg = ksh_ctx_get_arg(ctx, name);
    return arg && !arg->value.items;
}



static KshArg *ksh_ctx_get_arg(KshContext *ctx, StrView name)
{
    size_t args_count = ctx->args_count;
    KshArg *args = ctx->args;
    for (size_t i = 0; i < args_count; i++) {
        if (strv_eq(args[i].name, name)) {
            return &args[i];
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

static bool arg_predicate(Token tok)
{
    return tok.len > 2         &&
           tok.items[0] == '-' &&
           tok.items[1] == '-';
}
