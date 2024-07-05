
static const char *errmap[] = {
    [KSH_ERR_OK] = "ok",
    [KSH_ERR_COMMAND_NOT_FOUND] = "command not found",
    [KSH_ERR_TOKEN_EXPECTED] = "token was expected",
    [KSH_ERR_ARG_NOT_FOUND] = "arg not found",
    [KSH_ERR_VALUE_EXPECTED] = "value was expected",
    [KSH_ERR_PARSING_FAILED] = "parsing was failed"
};



const char *ksh_err_str(KshErr err)
{
    return errmap[err];
}
