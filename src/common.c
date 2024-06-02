#include "kovsh.h"
#include <string.h>


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

const char *ksh_val_type_str(KshValueType t)
{
    switch (t) {
        case KSH_VALUE_TYPE_STR:  return "string";
        case KSH_VALUE_TYPE_INT:  return "integer";
        case KSH_VALUE_TYPE_BOOL: return "boolean";
        default: assert(0 && "unknown value type");
    }
}
