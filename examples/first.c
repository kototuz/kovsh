#include "../src/kovsh.h"
#include <string.h>
#include <stdlib.h>

// A SIMPLE CONSOLE
int main(void)
{
    StrView prompt;
    CommandCall cmd_call;
    KshErr err;
    char *text;

    ksh_use_builtin_commands();

    ksh_init();

    for (;;) {
        printf(">>> ");
        getline(&text, &prompt.len, stdin);
        if (strncmp(text, "exit", 4) == 0) break;
        prompt.items = text;
        err = ksh_parse(prompt, &cmd_call);
        if (err != KSH_ERR_OK) {
            printf("ERROR: %d\n", err);
            return err;
        }
        err = ksh_cmd_call_execute(cmd_call);
        if (err != KSH_ERR_OK) {
            printf("ERROR: %d\n", err);
            return err;
        }
    }

    free(text);
    ksh_deinit();
}
