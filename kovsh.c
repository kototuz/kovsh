#include "kovsh.h"

#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef enum {
    KSH_ARG_KIND_FLAG,
    KSH_ARG_KIND_PARAM,
    KSH_ARG_KIND_SUBCMD
} KshArgKind;

typedef struct {
    uint8_t *items;
    size_t count;
} Bytes;

typedef bool (*ParamParser)(Token t, void *res, size_t idx);
typedef bool (*Handler)(void *self, KshParser *p);

typedef struct {
    const char *tostr;
    Handler handler;
} KshArgKindInfo;

typedef struct {
    const char *tostr;
    ParamParser parser;
} KshParamTypeInfo;



static bool lex_peek(Lexer *l);
static bool lex_next(Lexer *l);
static bool lex_next_if(Lexer *l, Token expected);
static bool lex_next_if_pred(Lexer *l, bool (*predicate)(Token));

static bool is_multiopt(StrView sv);

static bool isstr(int s);
static bool isend(int s);
static bool isbound(int s);

static KshArg *find_arg(Bytes bts, size_t itsize, StrView name);

static bool param_handler(KshParam *self, KshParser *p);
static bool flag_handler(KshFlag *self, KshParser *p);
static bool subcmd_handler(KshSubcmd *self, KshParser *p);

static bool str_parser(Token t, StrView *re, size_t idx);
static bool int_parser(Token t, int *res, size_t idx);

static bool store_bool_fh(bool *self, KshParser *p);
static bool help_fh(const char *self, KshParser *p);



static const Handler flag_handlers[] = {
    [KSH_FLAG_TYPE_STORE_BOOL] = (Handler) store_bool_fh,
    [KSH_FLAG_TYPE_HELP]       = (Handler) help_fh
};

static const KshParamTypeInfo param_types[] = {
    [KSH_PARAM_TYPE_STR] = { "str", (ParamParser) str_parser },
    [KSH_PARAM_TYPE_INT] = { "int", (ParamParser) int_parser }
};

static const KshArgKindInfo arg_kinds[] = {
    [KSH_ARG_KIND_PARAM]  = { "parameter", (Handler) param_handler },
    [KSH_ARG_KIND_FLAG]   = { "flag", (Handler) flag_handler },
    [KSH_ARG_KIND_SUBCMD] = { "subcommand", (Handler) subcmd_handler }
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



int ksh_parser_parse_cmd(KshParser *parser, KshCommandFn root, StrView input)
{
    parser->err[0] = '\0';
    parser->lex = (Lexer){ .text = input };
    return root(parser);
}

bool ksh_parse_cmd(KshParser *p, StrView cmd)
{
    p->err[0] = '\0';
    p->lex = (Lexer){ .text = cmd };
    p->params = (KshParams){0};
    p->flags = (KshFlags){0};
    p->subcmds = (KshSubcmds){0};
    p->root(p);
    return true;
}

bool ksh_parse(KshParser *p)
{
    StrView arg_name;
    KshArgKind arg_kind;
    size_t item_size;
    union {
        KshParams as_params;
        KshFlags as_flags;
        KshSubcmds as_subcmds;
        Bytes as_bytes;
    } source;

    while (lex_next(&p->lex)) {
        arg_name = p->lex.cur_tok;
        if (arg_name.items[0] == '-') {
            if (lex_peek(&p->lex) && p->lex.cur_tok.items[0] != '-') {
                item_size = sizeof(KshParam);
                source.as_params = p->params;
                arg_kind = KSH_ARG_KIND_PARAM;
            } else {
                item_size = sizeof(KshFlag);
                source.as_flags = p->flags;
                arg_kind = KSH_ARG_KIND_FLAG;
            }
            if (arg_name.items[1] == '-' && arg_name.len > 3) {
                arg_name.items += 2;
                arg_name.len -= 2;
            } else if (arg_name.len == 2) {
                arg_name.items += 1;
                arg_name.len = 1;
            }
        } else {
            source.as_subcmds = p->subcmds;
            arg_kind = KSH_ARG_KIND_SUBCMD;
            item_size = sizeof(KshSubcmd);
        }

        const KshArgKindInfo arg_kind_info = arg_kinds[arg_kind];
        KshArg *arg = find_arg(source.as_bytes, item_size, arg_name);
        if (!arg) {
            sprintf(p->err,
                    "%s `"STRV_FMT"` not found",
                    arg_kind_info.tostr,
                    STRV_ARG(arg_name));
            return false;
        }

        if (!arg_kind_info.handler(arg, p)) return false;
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

static bool is_multiopt(StrView sv)
{
    return sv.len > 2         &&
           sv.items[0] == '-' &&
           sv.items[1] != '-';
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

static KshArg *find_arg(Bytes bts, size_t itsize, StrView name)
{
    for (size_t i = 0; i < bts.count; i++) {
        bts.items += i*itsize;
        if (strv_eq(name, ((KshArg*)bts.items)->name))
            return (KshArg*)bts.items;
    }

    return NULL;
}



static bool param_handler(KshParam *self, KshParser *p)
{
    if (!lex_next(&p->lex)) {
        sprintf(p->err, "at least 1 param is required");
        return false;
    }

    KshParamTypeInfo pt_info = param_types[self->type];
    size_t count = self->max-1;
    do {
        if (!pt_info.parser(p->lex.cur_tok, self->var, count)) {
            sprintf(p->err,
                    "`"STRV_FMT"` is not a %s",
                    STRV_ARG(p->lex.cur_tok),
                    pt_info.tostr);
            return false;
        }
    } while (
        count-- != 0                   &&
        lex_next(&p->lex)              &&
        p->lex.cur_tok.items[0] != '-' 
    );

    return true;
}

static bool flag_handler(KshFlag *self, KshParser *p)
{
    return flag_handlers[self->type](self->var, p);
}

static bool subcmd_handler(KshSubcmd *self, KshParser *p)
{
    self->fn(p);
    return false;
}

static bool str_parser(Token t, StrView *res, size_t idx)
{
    res[idx] = t;
    if (isstr(t.items[0])) {
        res[idx].items += 1;
        res[idx].len -= 2;
    }

    return true;
}

static bool int_parser(Token t, int *res, size_t idx)
{
    int num = 0;
    for (size_t i = 0; i < t.len; i++) {
        if (!isdigit(t.items[i])) return false;
        num = num*10 + t.items[i]-'0';
    }

    res[idx] = num;
    return true;
}

static bool store_bool_fh(bool *self, KshParser *p)
{
    (void) p;
    return (*self = true);
}

static bool help_fh(const char *self, KshParser *p)
{
    (void) p;
    printf("[description]: %s\n", self);
    return false;
}
