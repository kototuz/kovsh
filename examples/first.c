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

    ksh_add_command((Command){
        .name = STRV_LIT("some"),
        .fn = some
    });

    prompt = (StrView)STRV_LIT("set da hello");
    err = ksh_parse(prompt, &cmd_call);
    if (err != KSH_ERR_OK) {
        puts("ERROR");
        return err;
    }
    ksh_cmd_call_exec(cmd_call);

    prompt = (StrView)STRV_LIT("print @da");
    err = ksh_parse(prompt, &cmd_call);
    if (err != KSH_ERR_OK) {
        puts("ERROR");
        return err;
    }
    ksh_cmd_call_exec(cmd_call);

    ksh_deinit();
}
