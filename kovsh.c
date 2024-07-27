#include "kovsh.h"

#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <setjmp.h>

typedef enum {
    KSH_ARG_KIND_PARAM,
    KSH_ARG_KIND_OPT_PARAM,

    KSH_ARG_KIND_FLAG,
    KSH_ARG_KIND_CHOICE,

    KSH_ARG_KIND_SUBCMD,

    KSH_ARG_KIND_COUNT
} KshArgKind;

typedef enum {
    KSH_EXIT_ERR = 1,
    KSH_EXIT_EARLY
} KshExit;

typedef union {
    void *as_opaque;
    KshArgBase *as_base;
    KshParam *as_param;
    KshFlag *as_flag;
    KshChoice *as_choice;
    KshSubcmd *as_subcmd;
} KshArgPtr;

typedef struct {
    uint8_t *items;
    size_t count;
} Bytes;

typedef struct {
    void *items;
    size_t count;
} KshArgGroup;

typedef bool (*ParamParser)(Token t, void *res, size_t idx);
typedef struct {
    const char *tostr;
    ParamParser parser;
} KshParamTypeInfo;

typedef void (*HelpFn)(void *self);
typedef struct {
    size_t struct_size;
    HelpFn help_fn;
    const char *str;
} KshArgKindInfo;

typedef bool (*EqFn)(void *arg, StrView name);

// TODO: refactor the lexer
static bool ksh_lex_peek(KshLexer *l, StrView *res);
static bool ksh_lex_next(KshLexer *l, StrView *res);
static bool lex_str_peek(char **data, StrView *res);

static bool isstr(int s);
static bool isend(int s);
static bool isbound(int s);

static KshArgPtr find_arg(Bytes *args, Token t, KshArgKind *result_kind);
static KshArgKind arg_actual(Token *arg);
static EqFn arg_name_eq_fn(KshArgKind kind);

static bool default_arg_name_eq(KshArgBase *arg, StrView name);
static bool choice_arg_name_eq(KshChoice *arg, StrView name);

static void parse_param_val(KshParam *param, KshParser *p);
static bool str_parser(Token t, StrView *re, size_t idx);
static bool int_parser(Token t, int *res, size_t idx);
static bool float_parser(Token t, float *res, size_t idx);

static void print_help(KshArgs *args);
static void print_arg_usages(Bytes *groups);
static void param_help(KshParam *self);
static void flag_help(KshFlag *self);
static void choice_help(KshChoice *self);
static void subcmd_help(KshSubcmd *self);



static jmp_buf ksh_exit;

// TODO: maybe we need to leave only KSH_PARAM_TYPE_CSTR
static const KshParamTypeInfo param_type[] = {
    [KSH_PARAM_TYPE_STR]   = { "<str>", (ParamParser) str_parser },
    [KSH_PARAM_TYPE_CSTR]  = { "<str>", NULL },
    [KSH_PARAM_TYPE_INT]   = { "<int>", (ParamParser) int_parser },
    [KSH_PARAM_TYPE_FLOAT] = { "<float>", (ParamParser) float_parser }
};

// TODO: remove the name. `arg_kind_info` already has it
static const struct {
    const char *name;
    size_t size;
} arg_group[] = {
    [KSH_ARG_KIND_PARAM]  = { "parameter", 2 },
    [KSH_ARG_KIND_FLAG]   = { "flag", 2 },
    [KSH_ARG_KIND_SUBCMD] = { "subcommand", 1 }
};


static const KshArgKindInfo arg_kind_info[] = {
    [KSH_ARG_KIND_PARAM]     = { sizeof(KshParam),  (HelpFn) param_help, "parameter" },
    [KSH_ARG_KIND_OPT_PARAM] = { sizeof(KshParam),  (HelpFn) param_help, "optional parameter" },
    [KSH_ARG_KIND_FLAG]      = { sizeof(KshFlag),   (HelpFn) flag_help, "flag" },
    [KSH_ARG_KIND_CHOICE]    = { sizeof(KshChoice), (HelpFn) choice_help, "choice" },
    [KSH_ARG_KIND_SUBCMD]    = { sizeof(KshSubcmd), (HelpFn) subcmd_help, "subcommand" },
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

    p->cmd_exit_code = root_cmd(p);

    return true;
}

void ksh_parse_args(KshParser *p, KshArgs *args)
{
    StrView arg_name;
    while (ksh_lex_next(&p->lex, &arg_name)) {
        if (strncmp(arg_name.items, "-help", 5) == 0) {
            print_help(args);
            longjmp(ksh_exit, KSH_EXIT_EARLY);
        }

        KshArgKind arg_kind;
        KshArgPtr arg = find_arg((Bytes*)args, arg_name, &arg_kind);
        if (!arg.as_opaque) {
            sprintf(p->err,
                    "%s `"STRV_FMT"` not found",
                    arg_group[arg_kind].name,
                    STRV_ARG(arg_name));
            longjmp(ksh_exit, KSH_EXIT_ERR);
        }

        switch (arg_kind) {
        case KSH_ARG_KIND_OPT_PARAM:
        case KSH_ARG_KIND_PARAM:
            parse_param_val(arg.as_param, p);
            break;

        case KSH_ARG_KIND_FLAG:
            *arg.as_flag->var = true;
            break;

        case KSH_ARG_KIND_CHOICE: break;

        case KSH_ARG_KIND_SUBCMD:
            p->cmd_exit_code = arg.as_subcmd->fn(p);
            longjmp(ksh_exit, KSH_EXIT_EARLY);
            break;

        default: assert(0 && "unreachable");
        }
    }

    KshParams params = args->params;
    for (size_t i = 0; i < params.count; i++) {
        if (params.items[i].var != NULL) {
            sprintf(p->err,
                    "parameter `"STRV_FMT"` must be assigned",
                    STRV_ARG(params.items[i].base.name));
            longjmp(ksh_exit, KSH_EXIT_ERR);
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

static KshArgPtr find_arg(Bytes *args, Token t, KshArgKind *result_kind)
{
    KshArgKind begin = arg_actual(&t);
    KshArgKind end = begin + arg_group[begin].size;

    *result_kind = begin;

    for (; begin < end; begin++) {
        Bytes bytes = args[begin];
        EqFn eq_fn = arg_name_eq_fn(begin);
        size_t item_size = arg_kind_info[begin].struct_size;
        for (; bytes.count > 0; bytes.count--, bytes.items += item_size)
        {
            if (eq_fn(bytes.items, t)) {
                *result_kind = begin;
                return (KshArgPtr){ .as_opaque = bytes.items };
            }
        }
    }

    return (KshArgPtr){ .as_opaque = NULL };
}

static void parse_param_val(KshParam *self, KshParser *p)
{
    StrView val;
    size_t max;

    if (!ksh_lex_next(&p->lex, &val)) {
        sprintf(p->err, "at least 1 param is required");
        longjmp(ksh_exit, KSH_EXIT_ERR);
    }

    if (self->type == KSH_PARAM_TYPE_CSTR) {
        max = self->max;
        strncpy(self->var, val.items,
                max > val.len ? val.len : max);
        return;
    }

    KshParamTypeInfo pt_info = param_type[self->type];
    max = self->max-1;
    do {
        if (!pt_info.parser(val, self->var, max)) {
            sprintf(p->err,
                    "`"STRV_FMT"` is not a %s",
                    STRV_ARG(val),
                    pt_info.tostr);
            longjmp(ksh_exit, KSH_EXIT_ERR);
        }
    } while (
        max-- != 0                   &&
        ksh_lex_next(&p->lex, &val)    &&
        val.items[0] != '-' 
    );

    self->var = NULL;
}

static KshArgKind arg_actual(StrView *an)
{
    switch (an->items[0]) {
    case '-':
        an->items++;
        an->len--;
        return KSH_ARG_KIND_FLAG;
    case '+':
        an->items++;
        an->len--;
        return KSH_ARG_KIND_PARAM;
    default: return KSH_ARG_KIND_SUBCMD;
    }
}

static EqFn arg_name_eq_fn(KshArgKind kind)
{
    if (kind == KSH_ARG_KIND_CHOICE) {
        return (EqFn)choice_arg_name_eq;
    } else {
        return (EqFn)default_arg_name_eq;
    }
}

static bool default_arg_name_eq(KshArgBase *arg, StrView name)
{
    return strv_eq(name, arg->name);
}

static bool choice_arg_name_eq(KshChoice *arg, StrView name)
{
    const char **names = arg->names;
    for (int i = 0; *names != NULL; names++, i++) {
        if (strv_eq(name, strv_from_str(*names))) {
            *(int*)arg->var = i;
            return true;
        }
    }

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

static void print_help(KshArgs *args)
{
    printf("[descr]:\n   %s\n", args->help);
    print_arg_usages((Bytes*)args);
}

static void print_arg_usages(Bytes *groups)
{
    for (KshArgKind i = 0; i < KSH_ARG_KIND_COUNT; i++) {
        Bytes group = groups[i];
        KshArgKindInfo ak_info = arg_kind_info[i];
        if (group.count == 0) continue;
        printf("\n[%ss]", ak_info.str);
        for (size_t y = 0;
             y < group.count;
             group.items += ++y * ak_info.struct_size)
        {
            printf("\n   ");
            ak_info.help_fn(group.items);
        }
        putchar('\n');
    }
}

static void param_help(KshParam *self)
{
    printf("+"STRV_FMT" %-10s %s",
           STRV_ARG(self->base.name),
           param_type[self->type].tostr,
           self->base.usage);
}

static void flag_help(KshFlag *self)
{
    printf("%-10s %s",
           self->base.name.items,
           self->base.usage);
}

static void choice_help(KshChoice *self)
{
    printf("-[");
    const char **names = self->names;
    assert(*names);
    printf(*names++);
    for (; *names != NULL; names++) {
        putchar('|');
        printf(*names);
    }
    printf("]\t%s", self->usage);
}

static void subcmd_help(KshSubcmd *self)
{
    printf("%-10s %s",
           self->base.name.items,
           self->base.usage);
}
