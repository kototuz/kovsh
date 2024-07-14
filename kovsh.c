#include "kovsh.h"

#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef enum {
    KSH_ARG_KIND_PARAM,
    KSH_ARG_KIND_FLAG,
    KSH_ARG_KIND_SUBCMD,
    KSH_ARG_KIND_COUNT
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
    size_t size;
} KshArgKindInfo;

typedef struct {
    const char *tostr;
    ParamParser parser;
} KshParamTypeInfo;



// TODO: refactor the lexer
static bool lex_peek(KshLexer *l);
static bool lex_next(KshLexer *l);

static bool isstr(int s);
static bool isend(int s);
static bool isbound(int s);

static void parser_reset_args(KshParser *self);
static KshArg *parser_find_arg(KshParser *self, KshArgKind kind, StrView name);

static bool param_handler(KshParam *self, KshParser *p);
static bool flag_handler(KshFlag *self, KshParser *p);
static bool subcmd_handler(KshSubcmd *self, KshParser *p);

static bool str_parser(Token t, StrView *re, size_t idx);
static bool int_parser(Token t, int *res, size_t idx);
static bool float_parser(Token t, float *res, size_t idx);

static bool store_bool_fh(bool *self, KshParser *p);
static bool help_fh(const char *self, KshParser *p);

static void print_param_usage(KshParam p);
static void print_flag_usage(KshFlag f);
static void print_subcmd_usage(KshSubcmd s);



static const Handler flag_handlers[] = {
    [KSH_FLAG_TYPE_STORE_BOOL] = (Handler) store_bool_fh,
    [KSH_FLAG_TYPE_HELP]       = (Handler) help_fh
};

static const KshParamTypeInfo param_types[] = {
    [KSH_PARAM_TYPE_STR]   = { "<str>", (ParamParser) str_parser },
    [KSH_PARAM_TYPE_INT]   = { "<int>", (ParamParser) int_parser },
    [KSH_PARAM_TYPE_FLOAT] = { "<float>", (ParamParser) float_parser }
};

static const KshArgKindInfo arg_kinds[] = {
    [KSH_ARG_KIND_PARAM]  = { "parameter", (Handler) param_handler, sizeof(KshParam) },
    [KSH_ARG_KIND_FLAG]   = { "flag", (Handler) flag_handler, sizeof(KshFlag) },
    [KSH_ARG_KIND_SUBCMD] = { "subcommand", (Handler) subcmd_handler, sizeof(KshSubcmd) }
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
    parser->lex = (KshLexer){ .text = input };
    return root(parser);
}

bool ksh_parse_cmd(KshParser *p, StrView cmd)
{
    p->err[0] = '\0';
    p->lex = (KshLexer){ .text = cmd };
    parser_reset_args(p);
    p->root(p);
    return true;
}

bool ksh_parse(KshParser *p)
{
    StrView arg_name;
    KshArgKind arg_kind;

    while (lex_next(&p->lex)) {
        arg_name = p->lex.cur_tok;
        if (arg_name.items[0] == '-') {
            arg_name.items += 1;
            arg_name.len -= 1;
            if (lex_peek(&p->lex) && p->lex.cur_tok.items[0] != '-') {
                arg_kind = KSH_ARG_KIND_PARAM;
            } else {
                arg_kind = KSH_ARG_KIND_FLAG;
            }
        } else {
            arg_kind = KSH_ARG_KIND_SUBCMD;
        }

        const KshArgKindInfo ak_info = arg_kinds[arg_kind];
        KshArg *arg = parser_find_arg(p, arg_kind, arg_name);
        if (!arg) {
            sprintf(p->err,
                    "%s `"STRV_FMT"` not found",
                    ak_info.tostr,
                    STRV_ARG(arg_name));
            return false;
        }

        if (!ak_info.handler(arg, p)) return false;
    }

    parser_reset_args(p);
    return true;
}



static bool lex_peek(KshLexer *l)
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

static bool lex_next(KshLexer *l)
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

static void parser_reset_args(KshParser *self)
{
    self->params = (KshParams){0};
    self->flags = (KshFlags){0};
    self->subcmds = (KshSubcmds){0};
}

static KshArg *parser_find_arg(KshParser *self, KshArgKind kind, StrView name)
{
    // KshParser {
    //   KshLexer lex;
    //   KshParams params;   \ <- as arrays;
    //   KshFlags flags;     |
    //   KshSubcmds subcmds; /
    //   KshCommandFn root;
    //   KshErr err;
    // }  
    Bytes grp = (*((Bytes (*)[KSH_ARG_KIND_COUNT])&self->params))[kind];
    size_t itsize = arg_kinds[kind].size;
    for (size_t i = 0; i < grp.count; i++) {
        grp.items += i*itsize;
        if (strv_eq(name, ((KshArg*)grp.items)->name)) {
            return (KshArg*)grp.items;
        }
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
    parser_reset_args(p);
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

static bool float_parser(Token t, float *res, size_t idx)
{
    float num = 0.0;
    bool int_part = true;
    float scale = 1;

    for (size_t i = 0; i < t.len; i++) {
        if (t.items[i] == '.') {
            int_part = false;
            continue;
        }

        if (!isdigit(t.items[i])) return false;
        if (int_part) {
            num = num*10 + t.items[i]-'0';
        } else {
            scale = scale/10;
            num = num + (t.items[i]-'0')*scale;
        }
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

    size_t ctr;
    union {
        KshParams as_ps;
        KshFlags as_fs;
        KshSubcmds as_ss;
    } args;

    printf("[description]\n   %s\n", self);


    args.as_ps = p->params;
    if (args.as_ps.count > 0) {
        putchar('\n');
        printf("[parameters]\n");
        for (ctr = 0; ctr < args.as_ps.count; ctr++) {
            printf("   ");
            print_param_usage(args.as_ps.items[ctr]);
        }
    }

    args.as_fs = p->flags;
    if (args.as_fs.count > 0) {
        putchar('\n');
        printf("[flags]\n");
        for (ctr = 0; ctr < args.as_fs.count; ctr++) {
            printf("   ");
            print_flag_usage(args.as_fs.items[ctr]);
        }
    }

    args.as_ss = p->subcmds;
    if (args.as_ss.count > 0) {
        putchar('\n');
        printf("[subcmds]\n");
        for (ctr = 0; ctr < args.as_ss.count; ctr++) {
            printf("   ");
            print_subcmd_usage(args.as_ss.items[ctr]);
        }
    }

    return false;
}

static void print_param_usage(KshParam p)
{
    printf("-%s %-10s%s\n",
           p.base.name.items,
           param_types[p.type].tostr,
           p.base.usage);
}

static void print_flag_usage(KshFlag f)
{
    printf("-%-12s%s\n", f.base.name.items, f.base.usage);
}

static void print_subcmd_usage(KshSubcmd s)
{
    printf("%-13s%s\n", s.base.name.items, s.base.usage);
}
