#include "../src/kovsh.h"

static int some(KshValue *args)
{
    (void) args;
    puts("Hello");
    return 0;
}

int main(void)
{
    ksh_init();

    ksh_add_command((Command){
        .name = STRV_LIT("some"),
        .fn = some
    });

    StrView prompt = STRV_LIT("some");

    CommandCall cmd_call;
    KshErr err = ksh_parse(prompt, &cmd_call);
    if (err != KSH_ERR_OK) return err;

    ksh_cmd_call_exec(cmd_call);

    ksh_deinit();
}
