
typedef bool (*ParseFn)(StrView in, void *res);

typedef struct {
    StrView data;
    bool is_multiopt;
} ArgName;



static KshArgDef *get_arg_def(KshArgDefs arg_defs, StrView name);
static bool arg_name_from_tok(Token tok, ArgName *res);
static const char *arg_name_prefix(StrView name);

static bool parse_str(StrView in, StrView *res);
static bool parse_int(StrView in, int *res);



static const ParseFn parsemap[] = {
    [KSH_ARG_KIND_PARAM_STR] = (ParseFn) parse_str,
    [KSH_ARG_KIND_PARAM_INT] = (ParseFn) parse_int
};



KshErr ksh_parse_cmd(StrView input, KshCommandFn root)
{
    root(&(Lexer){ .text = input });
    return KSH_ERR_OK;
}

KshErr ksh_parse_args_(Lexer *l, KshArgDefs arg_defs)
{
    KshArgDef *arg_def;
    ArgName arg_name;

    while (lex_next(l)) {
        if (!arg_name_from_tok(l->cur_tok, &arg_name)) return KSH_ERR_TOKEN_EXPECTED;

        if (arg_name.is_multiopt) {
            for (size_t i = 0; i < arg_name.data.len; i++) {
                arg_def = get_arg_def(arg_defs, (StrView){ 1, &arg_name.data.items[i] });
                if (!arg_def || arg_def->kind != KSH_ARG_KIND_OPT) return KSH_ERR_ARG_NOT_FOUND;
                *((bool*)arg_def->data.as_ptr) = true;
            }
            continue;
        }

        arg_def = get_arg_def(arg_defs, arg_name.data);
        if (!arg_def) return KSH_ERR_ARG_NOT_FOUND;

        if (IS_PARAM(arg_def->kind)) {
            if (!lex_next(l)) return KSH_ERR_VALUE_EXPECTED;
            if (!parsemap[arg_def->kind](l->cur_tok, arg_def->data.as_ptr))
                return KSH_ERR_PARSING_FAILED;
        } else if (arg_def->kind == KSH_ARG_KIND_OPT) {
            *((bool*)arg_def->data.as_ptr) = true;
        } else if (arg_def->kind == KSH_ARG_KIND_HELP) {
            printf("[descr]: %s\n", (char *)arg_def->data.as_ptr);
            for (size_t i = 0; i < arg_defs.count; i++) {
                printf("  %2s%-15s%s\n",
                       arg_name_prefix(arg_defs.items[i].name),
                       arg_defs.items[i].name.items,
                       arg_defs.items[i].usage);
            }
            return KSH_ERR_EARLY_EXIT;
        } else if (arg_def->kind == KSH_ARG_KIND_SUBCMD) {
            arg_def->data.as_fn(l);
            return KSH_ERR_EARLY_EXIT;
        }
    }

    return KSH_ERR_OK;
}



static bool arg_name_from_tok(Token tok, ArgName *res)
{
    if (tok.items[0] != '-') {
        *res = (ArgName){ .data = tok };
    } else if (tok.items[1] == '-') {
        if (tok.len <= 3) return false;
        *res = (ArgName){
            .data.items = &tok.items[2],
            .data.len = tok.len-2
        };
    } else if (tok.len > 2) {
        *res = (ArgName){
            .data.items = &tok.items[1],
            .data.len = tok.len-1,
            .is_multiopt = true
        };
    } else if (tok.len == 2) {
        *res = (ArgName){
            .data.items = &tok.items[1],
            .data.len = 1,
        };
    } else return false;

    return true;
}

static const char *arg_name_prefix(StrView name)
{
    return name.len > 1 ? "--" : "-";
}

static KshArgDef *get_arg_def(KshArgDefs arg_defs, StrView name)
{
    for (size_t i = 0; i < arg_defs.count; i++) {
        if (strv_eq(name, arg_defs.items[i].name)) {
            return &arg_defs.items[i];
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
