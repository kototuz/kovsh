#include "kovsh.h"

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
