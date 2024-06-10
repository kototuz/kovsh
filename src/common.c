#include "kovsh.h"
#include <string.h>
#include <stdlib.h>

typedef KshErr (*KshParseFn)(void *type_info, KshValue *value);

static KshErr parse_enum(KshValueTypeEnum *, KshValue *);
// TODO:
// static KshErr parse_array(StrView, KshArrayInfo *, KshValue *);

static const KshParseFn parsers[] = {
    [KSH_VALUE_TYPE_TAG_ENUM] = (KshParseFn) parse_enum
};

const char *ksh_err_str(KshErr err)
{
    switch (err) {
    case KSH_ERR_COMMAND_NOT_FOUND: return "command not found";
    case KSH_ERR_ARG_NOT_FOUND: return "arg not found";
    case KSH_ERR_TOKEN_EXPECTED: return "token was expected";
    case KSH_ERR_TYPE_EXPECTED: return "type was expected";
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

const char *ksh_val_type_tag_str(KshValueTypeTag t)
{
    switch (t) {
        case KSH_VALUE_TYPE_TAG_STR:  return "string";
        case KSH_VALUE_TYPE_TAG_INT:  return "integer";
        case KSH_VALUE_TYPE_TAG_BOOL: return "boolean";
        default: assert(0 && "unknown value type");
    }
}

KshErr ksh_val_parse(KshValueType type, KshValue *dest)
{
    if (type.info == NULL) return KSH_ERR_OK;
    return parsers[type.tag](type.info, dest);
}

KshErr parse_enum(KshValueTypeEnum *info, KshValue *value)
{
    for (size_t i = 0; i < info->cases_len; i++)
        if (strv_eq(value->as_str, info->cases[i])) {
            *value = (KshValue){ .as_int = i };
            return KSH_ERR_OK;
        }

    // TODO: add error type
    return 1;
}
