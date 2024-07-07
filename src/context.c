
typedef bool (*ParseFn)(StrView in, void *res);

typedef struct {
    StrView data;
    bool is_multiopt;
} ArgName;



static void print_arg_help(KshArgDef arg);
static KshArgDef *get_arg_def(KshArgDefs arg_defs, StrView name);
static bool arg_name_from_tok(Token tok, ArgName *res);

static bool parse_str(StrView in, StrView *res);
static bool parse_int(StrView in, int *res);



static const ParseFn parsemap[] = {
    [KSH_ARG_KIND_PARAM_STR] = (ParseFn) parse_str,
    [KSH_ARG_KIND_PARAM_INT] = (ParseFn) parse_int
};



bool ksh_parse_cmd(StrView input, KshCommandFn root)
{
    root(&(KshArgParser){ .lex = (Lexer){ .text = input } });
    return true;
}

bool ksh_parser_parse_args_(KshArgParser *parser, KshArgDefs arg_defs)
{
    KshArgDef *arg_def;
    ArgName arg_name;

    while (lex_next(&parser->lex)) {
        if (!arg_name_from_tok(parser->lex.cur_tok, &arg_name)) {
            sprintf(parser->err_msg,
                    "arg name was expected but found `"STRV_FMT"`\n",
                    STRV_ARG(parser->lex.cur_tok));
            return false;
        }

        if (arg_name.is_multiopt) {
            for (size_t i = 0; i < arg_name.data.len; i++) {
                arg_def = get_arg_def(arg_defs, (StrView){ 1, &arg_name.data.items[i] });
                if (!arg_def || arg_def->kind != KSH_ARG_KIND_OPT) {
                    sprintf(parser->err_msg,
                            "arg `%c` not found\n",
                            arg_name.data.items[i]);
                    return false;
                }
                *((bool*)arg_def->data.as_ptr) = true;
            }
            continue;
        }

        arg_def = get_arg_def(arg_defs, arg_name.data);
        if (!arg_def) {
            sprintf(parser->err_msg,
                    "arg `"STRV_FMT"` not found\n",
                    STRV_ARG(arg_name.data));
            return false;
        }

        if (IS_PARAM(arg_def->kind)) {
            if (!lex_next(&parser->lex)) {
                sprintf(parser->err_msg,
                        "arg `"STRV_FMT"` requires value\n",
                        STRV_ARG(arg_name.data));
                return false;
            }
            if (!parsemap[arg_def->kind](parser->lex.cur_tok, arg_def->data.as_ptr)) {
                sprintf(parser->err_msg,
                        "value `"STRV_FMT"` is not valid\n",
                        STRV_ARG(parser->lex.cur_tok));
                return false;
            }
        } else if (arg_def->kind == KSH_ARG_KIND_OPT) {
            *((bool*)arg_def->data.as_ptr) = true;
        } else if (arg_def->kind == KSH_ARG_KIND_HELP) {
            printf("[descr]: %s\n", (char *)arg_def->data.as_ptr);
            for (size_t i = 0; i < arg_defs.count; i++) {
                print_arg_help(arg_defs.items[i]);
                puts("");
            }
            parser->err_code = KSH_ERR_EARLY_EXIT;
            return false;
        } else if (arg_def->kind == KSH_ARG_KIND_SUBCMD) {
            arg_def->data.as_fn(parser);
            parser->err_code = KSH_ERR_EARLY_EXIT;
            return false;
        }
    }

    return true;
}



static void print_arg_help(KshArgDef arg)
{
    if (IS_PARAM(arg.kind) || arg.kind == KSH_ARG_KIND_OPT) {
        printf(arg.name.len > 2 ? "--" : " -");
    } else printf("  ");

    printf("%-15s%s", arg.name.items, arg.usage);
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
