#include "../cmd.h"
#include <stdbool.h>
#include <assert.h>

// PRIVATE FUNCTION DECLARATIONS
static void token_cursor_next(TokenCursor *tc);
static bool token_cursor_is_end(TokenCursor *tc);

static bool rule_vaarg_is_valid(Rule *rule, TokenCursor *tc);
static bool rule_oneof_is_valid(RuleArray *ra, TokenCursor *tc);
static bool rule_array_is_valid(RuleArray *ra, TokenCursor *tc);
static bool rule_simple_is_valid(TokenType tt, TokenCursor *tc);



// PUBLIC FUNCTIONS
Rule rule_vaarg(Rule *rule) {
    return (Rule) {
        .ptr = rule,
        .is_valid = (bool (*)(void *, TokenCursor *))rule_vaarg_is_valid
    };
}

Rule rule_oneof(RuleArray *rule_arr) {
    return (Rule) {
        .ptr = rule_arr,
        .is_valid = (bool (*)(void *, TokenCursor *))rule_oneof_is_valid,
    };
}

Rule rule_array(RuleArray *self) {
    return (Rule) {
        .ptr = self,
        .is_valid = (bool (*)(void *, TokenCursor *))rule_array_is_valid,
    };
}

Rule rule_simple(TokenType token_type) {
    return (Rule) {
        .ptr = (void *)token_type,
        .is_valid = (bool (*)(void *, TokenCursor *))rule_simple_is_valid,
    };
}

bool rule_is_valid(Rule *rule, TokenCursor *token_cursor) {
    return rule->is_valid(rule->ptr, token_cursor);
}

bool syntax_is_valid(TokenSeq *token_seq, RuleArray *rule_arr) {
    TokenCursor token_cursor = {
        .current = token_seq->begin,
        .max = token_seq->len
    };
    Rule rule = rule_array(rule_arr);
    return rule_is_valid(&rule, &token_cursor) && token_cursor_is_end(&token_cursor);
}

// PRIVATE FUNCTIONS
static void token_cursor_next(TokenCursor *self) {
    if (self->count >= self->max) {
        assert(0 && "[!] token_cursor_next: out of bounds");
    } else {
        self->count++;
        self->current = self->current->next;
    }
}

static bool token_cursor_is_end(TokenCursor *self) {
    return self->count >= self->max;
}


static bool rule_vaarg_is_valid(Rule *rule, TokenCursor *cursor) {
    while (rule_is_valid(rule, cursor) 
           && !token_cursor_is_end(cursor)) {
        token_cursor_next(cursor);
    }
    return true;
}

static bool rule_oneof_is_valid(RuleArray *rule_arr, TokenCursor *cursor) {
    size_t rules_len = rule_arr->len;
    for (size_t i = 0; i < rules_len; i++) {
        if (rule_is_valid(&rule_arr->rules[i], cursor)) {
            return true;
        }
    }

    return false;
}

static bool rule_array_is_valid(RuleArray *self, TokenCursor *cursor) {
    size_t arr_len = self->len;
    TokenNode *checkpoint = cursor->current;
    for (size_t i = 0; i < arr_len; i++) {
        if (!rule_is_valid(&self->rules[i], cursor)) {
            cursor->current = checkpoint;
            return false;
        }
    }

    return true;
}

static bool rule_simple_is_valid(TokenType token_type, TokenCursor *token_cursor) {
    if (token_cursor->current->val.type == token_type) {
        if (!token_cursor_is_end(token_cursor)) {
            token_cursor_next(token_cursor);
        }
        return true;
    }
    return false;
}
