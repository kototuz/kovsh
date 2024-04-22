#include "../cmd.h"
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// PRIVATE FUNCTION DECLARATIONS
static bool eval_rule_is_valid(Evaluator *e, TokenCursor *cursor);



// PUBLIC FUNCTIONS
StrSlice strslice_from_bounds(char *begin, char *end)
{
    return (StrSlice) {
        .begin = begin,
        .len = end - begin,
    };
}

Evaluator eval_const(EvalResult const_result)
{
    return (Evaluator) { .result = const_result };
}

Evaluator eval_new(Evaluator *e, EvalFn eval_fn, Rule inner_rule)
{
    return (Evaluator) {
        .inner_rule = inner_rule,
        .eval_fn = eval_fn,
        .prev_eval = e,
        .result = NULL
    };
}

EvalResult eval_get(Evaluator *e)
{
    if (e->result == NULL) {
        fputs("[!] eval_get: evaluator result is null\n", stderr);
        return NULL;
    }

    return e->result;
}

Rule eval_rule(Evaluator *e)
{
    return (Rule) {
        .ptr = e,
        .is_valid = (bool (*)(void *, TokenCursor *))eval_rule_is_valid,
    };
}

// PROVIDE FUNCTIONS
EvalResult eval_cmd(EvalResult cmdline, StrSlice cmd_name)
{
    CmdList cmd_list = ((Cmdline *)cmdline)->config.cmdList;
    for (size_t i = 0; i < cmd_list.len; i++) {
        if (strncmp(cmd_name.begin, cmd_list.items[i].name, cmd_name.len) == 0) {
            return &cmd_list.items[i];
        }
    }

    return NULL;
}

EvalResult eval_cmd_arg(EvalResult cmd, StrSlice arg_name)
{
    CmdArgList cmd_arg_list = ((Cmd *)cmd)->callback.argList;
    for (size_t i = 0; i < cmd_arg_list.len; i++) {
        if (strncmp(arg_name.begin, cmd_arg_list.items[i].name, arg_name.len) == 0) {
            return &cmd_arg_list.items[i];
        }
    }

    return NULL;
}

EvalResult eval_cmd_arg_val(EvalResult cmd_arg, StrSlice val)
{
    (void)cmd_arg;
    (void)val;
    assert(0 && "not implemented");
}



// PRIVATE FUNCTIONS
static bool eval_rule_is_valid(Evaluator *e, TokenCursor *cursor)
{
    char *data_begin;
    char *data_end;

    data_begin = cursor->current->val.data;
    if (!rule_is_valid(&e->inner_rule, cursor)) return false;

    data_end = cursor->current->val.data + cursor->current->val.data_len;
    e->eval_fn(eval_get(e->prev_eval), strslice_from_bounds(data_begin, data_end));

    return true;
}
