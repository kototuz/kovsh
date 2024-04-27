#ifndef KOVSH_H
#define KOVSH_H

#include <stddef.h>
#include <stdbool.h>

// UTILS
typedef struct {
    size_t len;
    const char *items;
} StrSlice;

StrSlice strslice_from_bounds(char *begin, char *end);
void strslice_new(StrSlice *ss, size_t len, char items[*]);

///////////////////////////////////////////////
/// LEXER
//////////////////////////////////////////////

typedef struct {
    StrSlice text;
    size_t cursor;
    int cur_letter;
} Lexer;

typedef enum {
    TOKEN_TYPE_LIT,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_EQ,
    TOKEN_TYPE_END,
} TokenType;

typedef struct {
    TokenType type;
    StrSlice text;
} Token;

typedef struct TokenNode {
    Token val;
    struct TokenNode *next;
} TokenNode;

typedef struct {
    TokenNode *begin;
    TokenNode *end;
    size_t len;
} TokenSeq;

Lexer lexer_new(StrSlice ss);
const char *lexer_token_type_to_string(TokenType token_type);
TokenSeq lexer_tokenize(Lexer *self);
Token lexer_token_next(Lexer *l);
void lexer_inc(Lexer *l, size_t inc);
void lexer_expect_token_next(Lexer *l, TokenType expect);

/////////////////////////////////////
/// RULE
/////////////////////////////////////

typedef struct {
    TokenNode *current;
    size_t count;
    size_t max;
} TokenCursor;

typedef struct {
    void *ptr;
    bool (*is_valid)(void *self, TokenCursor *cursor);
} Rule;

typedef struct {
    Rule *rules;
    size_t len;
} RuleArray;

Rule rule_vaarg(Rule *rule);
Rule rule_oneof(RuleArray *rule_arr);
Rule rule_array(RuleArray *rule_arr);
Rule rule_simple(TokenType token_type);

bool rule_is_valid(Rule *rule, TokenCursor *tc);
bool syntax_is_valid(TokenSeq *token_seq, RuleArray *rule_arr);

/////////////////////////////////
/// EVALUATOR
/////////////////////////////////

typedef void *EvalResult;
typedef EvalResult (*EvalFn)(EvalResult er, StrSlice ss);

typedef struct Evaluator {
    Rule inner_rule;
    EvalResult result;
    EvalFn eval_fn;
    struct Evaluator *prev_eval;
} Evaluator;


Evaluator  eval_const(EvalResult const_result);
Evaluator  eval_new(Evaluator *e, EvalFn eval_fn, Rule inner_rule);
EvalResult eval_get(Evaluator *e);

Rule eval_rule(Evaluator *e);

// EVALUATE FUNCTIONS
EvalResult eval_cmd(EvalResult cmdline, StrSlice cmd_name);
EvalResult eval_cmd_arg(EvalResult cmd, StrSlice arg_name);
EvalResult eval_cmd_arg_val(EvalResult cmd_arg, StrSlice val);


//////////////////////////////
/// EXPRESSION
//////////////////////////////

typedef union {
    StrSlice as_str;
    int as_int;
} ExprResult;

typedef ExprResult (*ExprEvalFn)(void *self);

typedef struct {
    void *ptr;
    ExprEvalFn fn;
} Expr;


void expr_parse(TokenCursor *token_cursor);


#endif
