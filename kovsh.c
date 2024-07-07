#include "kovsh.h"

#include <string.h>
#include <ctype.h>

typedef bool (*ParseFn)(StrView in, void *res);

typedef struct {
    StrView data;
    bool is_multiopt;
} ArgName;



static bool lex_peek(Lexer *l);
static bool lex_next(Lexer *l);
static bool lex_next_if(Lexer *l, Token expected);
static bool lex_next_if_pred(Lexer *l, bool (*predicate)(Token));

static bool isstr(int s);
static bool isend(int s);
static bool isbound(int s);

static void print_arg_help(KshArgDef arg);
static KshArgDef *get_arg_def(KshArgDefs arg_defs, StrView name);
static bool arg_name_from_tok(Token tok, ArgName *res);

static bool parse_str(StrView in, StrView *res);
static bool parse_int(StrView in, int *res);



static const ParseFn parsemap[] = {
    [KSH_ARG_KIND_PARAM_STR] = (ParseFn) parse_str,
    [KSH_ARG_KIND_PARAM_INT] = (ParseFn) parse_int
};



StrView strv_new(const char *data, size_t data_len)
{
    assert(strlen(data) >= data_len);
    return (StrView){ .items = data, .len = data_len };
}

StrView strv_from_str(const char *data)
{
    return (StrView){ .items = data, .len = strlen(data) };
}

bool strv_eq(StrView sv1, StrView sv2)
{
    return sv1.len == sv2.len &&
           (memcmp(sv1.items, sv2.items, sv1.len) == 0);
}



int ksh_parser_parse_cmd(KshArgParser *parser, KshCommandFn root, StrView input)
{
    parser->lex = (Lexer){ .text = input };
    return root(parser);
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
