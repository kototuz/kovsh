#include "kovsh.h"
#include <string.h>
#include <stdlib.h>

static KshErr str_parse_fn(StrView text, KshStrContext *context, KshValue *value);
static KshErr int_parse_fn(StrView text, KshIntContext *context, KshValue *value);
static KshErr enum_parse_fn(StrView text, KshEnumContext *context, KshValue *value);
static KshErr bool_parse_fn(StrView text, void *context, KshValue *value);

typedef KshErr (*ParseFn)(StrView text, void *context, KshValue *dest);

static const struct {
    ParseFn parse_fn;
    const char *name;
    TokenType *expected_ttypes;
    size_t expected_ttypes_len;
} types[] = {
    [KSH_VALUE_TYPE_TAG_STR]  = { 
        .parse_fn = (ParseFn) str_parse_fn,
        .name = "string",
        .expected_ttypes_len = 2,
        .expected_ttypes = (TokenType[]){
            TOKEN_TYPE_STRING,
            TOKEN_TYPE_LIT
        }
    },
    [KSH_VALUE_TYPE_TAG_INT]  = {
        .parse_fn = (ParseFn) int_parse_fn,
        .name = "integer",
        .expected_ttypes_len = 1,
        .expected_ttypes = (TokenType[]){TOKEN_TYPE_NUMBER}
    },
    [KSH_VALUE_TYPE_TAG_BOOL] = {
        .parse_fn = (ParseFn) bool_parse_fn,
        .name = "bool",
        .expected_ttypes_len = 1,
        .expected_ttypes = (TokenType[]){TOKEN_TYPE_BOOL}
    },
    [KSH_VALUE_TYPE_TAG_ENUM] = {
        .parse_fn = (ParseFn) enum_parse_fn,
        .name = "enum",
        .expected_ttypes_len = 1,
        .expected_ttypes = (TokenType[]){TOKEN_TYPE_LIT}
    }
};
#define TYPES_COUNT (sizeof(types)/sizeof(types[0]))

const char *ksh_err_str(KshErr err)
{
    switch (err) {
    case KSH_ERR_COMMAND_NOT_FOUND: return "command not found";
    case KSH_ERR_ARG_NOT_FOUND: return "arg not found";
    case KSH_ERR_TOKEN_EXPECTED: return "token was expected";
    case KSH_ERR_UNDEFINED_TOKEN: return "token is undefined";
    case KSH_ERR_TYPE_EXPECTED: return "type was expected";
    case KSH_ERR_ASSIGNMENT_EXPECTED: return "arg must be assigned";
    case KSH_ERR_MEM_OVER: return "memory is over";
    case KSH_ERR_VAR_NOT_FOUND: return "variable not found";
    case KSH_ERR_NAME_ALREADY_EXISTS: return "name already exists";
    case KSH_ERR_CONTEXT: return "arg doesn't fit the context";
    default: return "unknown";
    }
}

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

bool ksh_val_type_eq_ttype(KshValueTypeTag type_tag, TokenType tt)
{
    if (type_tag >= TYPES_COUNT) return false;

    for (size_t i = 0; i < types[type_tag].expected_ttypes_len; i++) {
        if (types[type_tag].expected_ttypes[i] == tt) return true;
    }

    return false;
}

const char *ksh_val_type_tag_str(KshValueTypeTag t)
{
    switch (t) {
        case KSH_VALUE_TYPE_TAG_STR:  return "string";
        case KSH_VALUE_TYPE_TAG_INT:  return "integer";
        case KSH_VALUE_TYPE_TAG_BOOL: return "boolean";
        default: assert(0 && "unknown value type");
    }
}

KshErr ksh_val_parse(StrView text, KshValueTypeInst instance, KshValue *value)
{
    if (instance.type_tag >= TYPES_COUNT) return KSH_ERR_TYPE_EXPECTED;
    return types[instance.type_tag].parse_fn(text, instance.context, value);
}

static KshErr str_parse_fn(StrView text, KshStrContext *context, KshValue *value)
{
    if (context) {
        if (text.len > context->max_line_len ||
            text.len < context->min_line_len) {
            return KSH_ERR_CONTEXT;
        }
    }

    value->as_str = text;

    return KSH_ERR_OK;
}

static KshErr int_parse_fn(StrView text, KshIntContext *context, KshValue *value)
{
    char *buf = (char *) malloc(text.len);
    if (!buf) return KSH_ERR_MEM_OVER;

    memcpy(buf, text.items, text.len);
    int i = atoi(buf);
    free(buf);

    if (context) {
        if (i > context->max ||
            i < context->min) {
            return KSH_ERR_CONTEXT;
        }
    }

    value->as_int = i;
    return KSH_ERR_OK;
}

static KshErr bool_parse_fn(StrView text, void *context, KshValue *value)
{
    (void) context;

    value->as_bool = text.items[0] == 't' ? 1 : 0;
    return KSH_ERR_OK;
}

static KshErr enum_parse_fn(StrView text, KshEnumContext *context, KshValue *value)
{
    for (size_t i = 0; i < context->cases_len; i++) {
        if (strv_eq(text, context->cases[i])) {
            value->as_int = i;
            return KSH_ERR_OK;
        }
    }

    return KSH_ERR_CONTEXT;
}
