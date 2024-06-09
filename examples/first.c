#include "../src/kovsh.h"

int main(void)
{
    ksh_init();

    StrView prompt = STRV_LIT("print hello");

    CommandCall cmd_call;
    KshErr err = ksh_parse(prompt, &cmd_call);
    if (err != KSH_ERR_OK) return err;

    ksh_cmd_call_exec(cmd_call);

    ksh_deinit();
}
