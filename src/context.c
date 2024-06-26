
typedef bool (*ParseFn)(StrView in, void *res);



static KshArg *get_arg(KshContext ctx, StrView name);

static bool parse_str(StrView in, StrView *res);
static bool parse_int(StrView in, int *res);



static const ParseFn parsemap[] = {
    [KSH_PARAM_TYPE_STR] = (ParseFn) parse_str,
    [KSH_PARAM_TYPE_INT] = (ParseFn) parse_int
};



bool ksh_ctx_get_param(KshContext ctx,
                       StrView name,
                       KshParamType param_type,
                       void *res)
{
    KshArg *arg = get_arg(ctx, name);
    if (!arg) return false;

    return parsemap[param_type](arg->value, res);
}

bool ksh_ctx_get_option(KshContext ctx, StrView name)
{
    KshArg *arg = get_arg(ctx, name);
    return arg && !arg->value.items;
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

static KshArg *get_arg(KshContext ctx, StrView name)
{
    for (size_t i = 0; i < ctx.args_count; i++) {
        if (strv_eq(ctx.args[i].name, name)) {
            return &ctx.args[i];
        }
    }

    return NULL;
}
