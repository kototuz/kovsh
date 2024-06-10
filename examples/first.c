#include "../src/kovsh.h"

static int some(KshValue *args)
{
    (void) args;
    puts("Hello");
    return 0;
}

int main(void)
{
    StrView prompt;
    CommandCall cmd_call;
    KshErr err;

    ksh_init();

    prompt = (StrView)STRV_LIT("enum second");
    err = ksh_parse(prompt, &cmd_call);
    if (err != KSH_ERR_OK) {
        puts("ERROR");
        return err;
    }
    ksh_cmd_call_execute(cmd_call);

    prompt = (StrView)STRV_LIT("print hello");
    err = ksh_parse(prompt, &cmd_call);
    if (err != KSH_ERR_OK) {
        puts("ERROR");
        return err;
    }
    ksh_cmd_call_execute(cmd_call);

    ksh_deinit();
}
