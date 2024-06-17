#include "../src/kovsh.h"
#include <string.h>
#include <stdlib.h>

static int welcome(ArgValueCopy *args)
{
    (void) args;
    printf("You're welcom, "STRV_FMT"\n", STRV_ARG(args[0].value.data.as_str));
    printf("Value: %d\n", args[1].value.data.as_int);
    printf("Enum member: %d\n", args[2].value.data.as_int);
    return 0;
}

// A SIMPLE CONSOLE
int main(void)
{
    StrView prompt;
    CommandCall cmd_call;
    KshErr err;
    char *text;

    ksh_use_command_set((CommandSet){
        .len = 1,
        .items = (Command[]){
            {
                .name = STRV_LIT("welcome"),
                .fn = welcome,
                .args_len = 3,
                .args = (Arg[]){
                    {
                        .name = STRV_LIT("person"),
                        .type_inst.type_tag = KSH_VALUE_TYPE_TAG_STR,
                        .type_inst.context = &(KshStrContext){
                            .max_line_len = 11
                        }
                    },
                    {
                        .name = STRV_LIT("value"),
                        .type_inst.type_tag = KSH_VALUE_TYPE_TAG_INT,
                        .type_inst.context = &(KshIntContext){ .max = 10 }
                    },
                    {
                        .name = STRV_LIT("enum"),
                        .type_inst.type_tag = KSH_VALUE_TYPE_TAG_ENUM,
                        .type_inst.context = &(KshEnumContext){
                            .cases_len = 3,
                            .cases = (StrView[]){
                                STRV_LIT("first"),
                                STRV_LIT("second"),
                                STRV_LIT("third"),
                            }
                        }
                    }
                }
            }
        }
    });

    ksh_use_builtin_commands();

    ksh_init();

    for (;;) {
        printf(">>> ");
        getline(&text, &prompt.len, stdin);
        if (strncmp(text, "exit", 4) == 0) break;
        prompt.items = text;
        err = ksh_parse(prompt, &cmd_call);
        if (err != KSH_ERR_OK) {
            printf("ERROR: %s\n", ksh_err_str(err));
            return err;
        }
        err = ksh_cmd_call_execute(cmd_call);
        if (err != KSH_ERR_OK) {
            printf("ERROR: %s\n", ksh_err_str(err));
            return err;
        }
    }

    free(text);
    ksh_deinit();
}
