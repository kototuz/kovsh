#include "kovsh.h"

#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <setjmp.h>

typedef enum {
    KSH_ARG_KIND_PARAM,
    KSH_ARG_KIND_FLAG,
    KSH_ARG_KIND_SUBCMD,
    KSH_ARG_KIND_HELP
} KshArgKind;

typedef enum {
    KSH_EXIT_ERR = 1,
    KSH_EXIT_EARLY
} KshExit;

typedef struct {
    uint8_t *items;
    size_t count;
} Bytes;

typedef bool (*ParamParser)(Token t, void *res, size_t idx);
typedef struct {
    const char *tostr;
    ParamParser parser;
} KshParamTypeInfo;

// TODO: refactor the lexer
static bool ksh_lex_peek(KshLexer *l, StrView *res);
static bool ksh_lex_next(KshLexer *l, StrView *res);
static bool lex_str_peek(char **data, StrView *res);

static bool isstr(int s);
static bool isend(int s);
static bool isbound(int s);

static KshArg *args_find_arg(Bytes arr, size_t it_size, StrView name);
static void parse_param_val(KshParam *param, KshParser *p);
static KshArgKind parse_arg_name(StrView *an, size_t *item_size);

static bool str_parser(Token t, StrView *re, size_t idx);
static bool int_parser(Token t, int *res, size_t idx);
static bool float_parser(Token t, float *res, size_t idx);


static void print_help(const char *descr, KshArgs args);
static void print_param_usage(KshParam p);
static void print_flag_usage(KshFlag f);
static void print_subcmd_usage(KshSubcmd s);



static jmp_buf ksh_exit;

static const KshParamTypeInfo param_types[] = {
    [KSH_PARAM_TYPE_STR]   = { "<str>", (ParamParser) str_parser },
    [KSH_PARAM_TYPE_INT]   = { "<int>", (ParamParser) int_parser },
    [KSH_PARAM_TYPE_FLOAT] = { "<float>", (ParamParser) float_parser }
};

static const char *arg_kind_str[] = {
    [KSH_ARG_KIND_PARAM]  = "parameter",
    [KSH_ARG_KIND_FLAG]   = "flag",
    [KSH_ARG_KIND_SUBCMD] = "subcommand"
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

void ksh_init_from_cstr(KshParser *p, char *cstr)
{
    p->lex.kind = KSH_LEXER_KIND_CSTR;
    p->lex.src.as_cstr = cstr;
}

void ksh_init_from_cargs(KshParser *p, int argc, char **argv)
{
    p->lex.kind = KSH_LEXER_KIND_CARGS;
    argc--;
    argv++;
    p->lex.src.as_cargs.argc = argc;
    p->lex.src.as_cargs.argv = argv;
}

bool ksh_parse(KshParser *p, KshCommandFn root_cmd)
{
    switch (setjmp(ksh_exit)) {
    case KSH_EXIT_EARLY: return true;
    case KSH_EXIT_ERR:
        fprintf(stderr, "ERROR: %s\n", p->err);
        return false;
    default: break;
    }

    root_cmd(p);

    return true;
}

void ksh_parse_args(KshParser *p, KshArgs *args)
{
    StrView arg_name;
    while (ksh_lex_next(&p->lex, &arg_name)) {
        size_t item_size;
        KshArgKind arg_kind = parse_arg_name(&arg_name, &item_size);
        if (arg_kind == KSH_ARG_KIND_HELP) {
            print_help(args->help, *args);
            longjmp(ksh_exit, KSH_EXIT_EARLY);
        }

        KshArg *arg = args_find_arg(((Bytes*)args)[arg_kind],
                                    item_size,
                                    arg_name);
        if (!arg) {
            sprintf(p->err,
                    "%s `"STRV_FMT"` not found",
                        arg_kind_str[arg_kind],
                    STRV_ARG(arg_name));
            longjmp(ksh_exit, KSH_EXIT_ERR);
        }

        switch (arg_kind) {
        case KSH_ARG_KIND_PARAM:
            parse_param_val((KshParam*)arg, p);
            break;
        case KSH_ARG_KIND_FLAG:
            *((KshFlag*)arg)->var = true;
            break;
        case KSH_ARG_KIND_SUBCMD:
            ((KshSubcmd*)arg)->fn(p);
            longjmp(ksh_exit, KSH_EXIT_EARLY);
            break;
        default: assert(0 && "unreachable");
        }
    }
}



static bool ksh_lex_peek(KshLexer *l, StrView *res)
{
    KshLexerSource src = l->src;
    switch (l->kind) {
    case KSH_LEXER_KIND_CARGS:
        if (src.as_cargs.argc == 0) return false;
        res->items = *src.as_cargs.argv;
        res->len = strlen(*src.as_cargs.argv);
        return true;
    case KSH_LEXER_KIND_CSTR: return lex_str_peek(&l->src.as_cstr, res);
    default: assert(0 && "unreachable");
    }
}

static bool ksh_lex_next(KshLexer *l, StrView *res)
{
    if (!ksh_lex_peek(l, res)) return false;
    switch (l->kind) {
    case KSH_LEXER_KIND_CARGS:
        l->src.as_cargs.argv++;
        l->src.as_cargs.argc--;
        break;
    case KSH_LEXER_KIND_CSTR:
        l->src.as_cstr += res->len;
        break;
    }
    
    return true;
}

static bool lex_str_peek(char **data, StrView *res)
{
    char *text = *data;

    if (isspace(*text)) {
        while (isspace(*++text));
    }

    *data = text;

    if (isend(*text)) return false;

    size_t res_len = 0;
    if (isstr(*text)) {
        int pair_s = *text;
        while (text[++res_len] != pair_s)
            if (isend(text[res_len])) return false;
        res_len++;
    } else {
        while (!isbound(text[++res_len]));
    }

    res->items = text;
    res->len = res_len;
    return true;
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

static KshArg *args_find_arg(Bytes arr, size_t it_size, StrView name)
{
    for (size_t i = 0; i < arr.count; i++) {
        arr.items += i*it_size;
        if (strv_eq(name, ((KshArg*)arr.items)->name)) {
            return (KshArg*)arr.items;
        }
    }

    return NULL;
}

static void parse_param_val(KshParam *self, KshParser *p)
{
    StrView val;
    if (!ksh_lex_next(&p->lex, &val)) {
        sprintf(p->err, "at least 1 param is required");
        longjmp(ksh_exit, KSH_EXIT_ERR);
    }

    KshParamTypeInfo pt_info = param_types[self->type];
    size_t count = self->max-1;
    do {
        if (!pt_info.parser(val, self->var, count)) {
            sprintf(p->err,
                    "`"STRV_FMT"` is not a %s",
                    STRV_ARG(val),
                    pt_info.tostr);
            longjmp(ksh_exit, KSH_EXIT_ERR);
        }
        self->assigned = true;
    } while (
        count-- != 0                   &&
        ksh_lex_next(&p->lex, &val)    &&
        val.items[0] != '-' 
    );
}

static KshArgKind parse_arg_name(StrView *an, size_t *item_size)
{
    switch (an->items[0]) {
    case '-':
        if (strncmp(&an->items[1], "help", 4) == 0) {
            return KSH_ARG_KIND_HELP;
        }

        an->items++;
        an->len--;
        *item_size = sizeof(KshFlag);
        return KSH_ARG_KIND_FLAG;
    case '+':
        an->items++;
        an->len--;
        *item_size = sizeof(KshParam);
        return KSH_ARG_KIND_PARAM;
    default: return KSH_ARG_KIND_SUBCMD;
    }
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

static void print_help(const char *descr, KshArgs args)
{
    size_t ctr;
    union {
        KshParams as_ps;
        KshFlags as_fs;
        KshSubcmds as_ss;
    } grp;

    printf("[description]\n   %s\n", descr);


    grp.as_ps = args.params;
    if (grp.as_ps.count > 0) {
        putchar('\n');
        printf("[parameters]\n");
        for (ctr = 0; ctr < grp.as_ps.count; ctr++) {
            printf("   ");
            print_param_usage(grp.as_ps.items[ctr]);
        }
    }

    grp.as_fs = args.flags;
    if (grp.as_fs.count > 0) {
        putchar('\n');
        printf("[flags]\n");
        for (ctr = 0; ctr < grp.as_fs.count; ctr++) {
            printf("   ");
            print_flag_usage(grp.as_fs.items[ctr]);
        }
    }

    grp.as_ss = args.subcmds;
    if (grp.as_ss.count > 0) {
        putchar('\n');
        printf("[subcmds]\n");
        for (ctr = 0; ctr < grp.as_ss.count; ctr++) {
            printf("   ");
            print_subcmd_usage(grp.as_ss.items[ctr]);
        }
    }
}

static void print_param_usage(KshParam p)
{
    printf("+%s %-10s%s\n",
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
