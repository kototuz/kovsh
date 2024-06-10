#include "../src/kovsh.h"
#include <string.h>
#include <stdlib.h>

static int some(KshValue *args)
{
    (void) args;
    puts("Hello");
    return 0;
}

// A SIMPLE CONSOLE
int main(void)
{
    StrView prompt;
    CommandCall cmd_call;
    KshErr err;
    char *text;

    ksh_init();

    for (;;) {
        printf(">>> ");
        getline(&text, &prompt.len, stdin);
        if (strncmp(text, "exit", 4) == 0) break;
        prompt.items = text;
        err = ksh_parse(prompt, &cmd_call);
        if (err != KSH_ERR_OK) {
            puts("ERROR");
            return err;
        }
        ksh_cmd_call_execute(cmd_call);
    }

    free(text);
    ksh_deinit();
}
