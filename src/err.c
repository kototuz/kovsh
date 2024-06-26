
static const char *errmap[] = {
    [KSH_ERR_OK] = "ok",
    [KSH_ERR_COMMAND_NOT_FOUND] = "command not found",
    [KSH_ERR_TOKEN_EXPECTED] = "token was expected",
};



const char *ksh_err_str(KshErr err)
{
    return errmap[err];
}
