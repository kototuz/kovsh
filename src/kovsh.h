#ifndef KOVSH_H
#define KOVSH_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>

#define KSH_LOG_ERR(msg, ...) \
  (ksh_termio_print((TermTextPrefs){.fg_color = TERM_COLOR_RED, .mode.bold = true}, "[KOVSH ERR]: "msg"\n", __VA_ARGS__))

// UTILS
// ERRORS
typedef enum {
    KSH_ERR_OK = 0,
    KSH_ERR_COMMAND_NOT_FOUND,
    KSH_ERR_ARG_NOT_FOUND,
    KSH_ERR_TOKEN_EXPECTED,
    KSH_ERR_TYPE_EXPECTED,
    KSH_ERR_ASSIGNMENT_EXPECTED,
    KSH_ERR_MEM_OVER,
    KSH_ERR_PATTERN_NOT_FOUND
} KshErr;

const char *ksh_err_str(KshErr err);

typedef struct {
    size_t len;
    const char *items;
} StrView;
#define STRV_LIT(lit) { sizeof(lit)-1, (lit) }

#define STRV_FMT "%.*s"
#define STRV_ARG(sv) (int) (sv).len, (sv).items

StrView strv_new(const char *data, size_t data_len);
bool strv_eq(StrView sv1, StrView sv2);

///////////////////////////////////////////////
/// COMMAND
//////////////////////////////////////////////

typedef enum {
    ARG_VAL_TYPE_STR,
    ARG_VAL_TYPE_INT,
    ARG_VAL_TYPE_BOOL,
} ArgValType;

typedef union {
    StrView as_str;
    int as_int;
    bool as_bool;
} ArgVal; 

typedef struct {
    StrView name;
    const char *usage;
    bool has_default;
    ArgValType type;
    ArgVal default_val;
} ArgDef;

typedef struct {
    ArgDef *def;
    bool is_assign;
    ArgVal value;
} Arg;

typedef int (*CommandFn)(size_t argc, Arg argv[argc]);

typedef struct {
    StrView name;
    const char *desc;
    CommandFn fn;
    ArgDef *arg_defs;
    size_t arg_defs_len;
} Command;

typedef struct {
    Command *cmd;
    Arg *argv;
    size_t argc;
} CommandCall;

typedef enum {
    COMMAND_CALL_STATE_DEF,
    COMMAND_CALL_STATE_EXIT,
    COMMAND_CALL_STATE_SYS,
} CommandCallState;

typedef struct {
    Command *items;
    size_t len;
} CommandBuf;

typedef struct {
    CommandCall cmd_call;
    CommandCallState state;
    CommandBuf commands;
    size_t last_assigned_arg;
} CallContext;


void ksh_cmd_print(Command cmd);
Command *ksh_cmd_find_local(CommandBuf buf, StrView sv);
Command *ksh_cmd_find_hardcoded(StrView sv);
Command *ksh_cmd_find(CommandBuf buf, StrView sv);
CommandCall ksh_cmd_create_call(Command *cmd);
ArgDef *ksh_cmd_find_arg_def(Command *cmd, StrView name);

KshErr ksh_cmd_call_exec(CommandCall call);

const char *ksh_arg_val_type_to_str(ArgValType avt);
void ksh_arg_def_print(ArgDef cmd_arg);

Arg *ksh_args_find(size_t argc, Arg argv[argc], StrView sv);

///////////////////////////////////////////////
/// LEXER
//////////////////////////////////////////////


typedef enum {
    TOKEN_TYPE_LIT,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_EQ,
    TOKEN_TYPE_KEYWORD_SYS,
    TOKEN_TYPE_INVALID,

    TOKEN_TYPE_END,
    TOKEN_TYPE_ENUM_END,
    TOKEN_TYPE_PLUS
} TokenType;

typedef struct {
    TokenType type;
    StrView text;
} Token;

typedef struct {
    StrView text;
    size_t cursor;
    Token buf;
} Lexer;

typedef struct {
    TokenType *seq;
    size_t seq_len;
    KshErr (*eval_fn)(Token *toks, CommandCall *cmd_call);
} TokenPattern;

typedef struct {
    Token *items;
    size_t len;
} TokenSeq;

typedef struct {
    CommandBuf cmds;
} ParseInfo;

typedef struct {
    ParseInfo info;
    Lexer lex;
} Parser;

Lexer ksh_lexer_new(StrView sv);
const char *ksh_lexer_token_type_to_string(TokenType token_type);
bool ksh_lexer_peek_token(Lexer *l, Token *t);
bool ksh_lexer_next_token(Lexer *l, Token *t);
bool ksh_lexer_is_next_token(Lexer *l, TokenType tt);
bool ksh_lexer_next_token_if(Lexer *l, TokenType tt, Token *t);
KshErr ksh_lexer_expect_next_token(Lexer *l, TokenType expect, Token *out);

ArgVal ksh_token_to_arg_val(Token tok);

KshErr ksh_parse(Lexer *lex, CallContext context);

///////////////////////////////////////////////
/// TERM
//////////////////////////////////////////////

#define TERMIO_NCURSES 0
#define TERMIO_DEFAULT 1
#ifndef TERMIO
#  define TERMIO TERMIO_DEFAULT
#endif

#define TERM_COLOR_COUNT (TERM_COLOR_END-1)
typedef enum {
    TERM_COLOR_BLACK = 1,
    TERM_COLOR_RED,
    TERM_COLOR_GREEN,
    TERM_COLOR_YELLOW,
    TERM_COLOR_BLUE,
    TERM_COLOR_MAGENTA,
    TERM_COLOR_CYAN,
    TERM_COLOR_WHITE,

    TERM_COLOR_END
} TermColor;

#define TERM_TEXT_MODE_COUNT 5
typedef struct {
    bool bold;
    bool italic;
    bool underscored;
    bool blink;
    bool strikethrough;
} TermTextMode;
static_assert(TERM_TEXT_MODE_COUNT == (sizeof(TermTextMode) / sizeof(bool)), "unsupported modes");

typedef struct {
    TermColor fg_color;
    TermColor bg_color;
    TermTextMode mode;
} TermTextPrefs;

typedef struct {
    const char *text;
    TermTextPrefs text_prefs;
} PromptPart;

typedef struct {
    PromptPart *parts;
    size_t parts_len;
} Prompt;

typedef struct {
    CommandBuf cmd_buf;
    Prompt prompt;
} Terminal;

void ksh_term_start(Terminal term);

void ksh_prompt_print(Prompt p);

void ksh_termio_init(void);
void ksh_termio_print(TermTextPrefs prefs, const char *fmt, ...);
void ksh_termio_getline(size_t size, char buf[size]);
void ksh_termio_end(void);

#endif
