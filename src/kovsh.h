#ifndef KOVSH_H
#define KOVSH_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>

///////////////////////////////////////////////
/// COMMON
//////////////////////////////////////////////

#define KSH_LOG_ERR(msg, ...) \
  (ksh_termio_print((TermTextPrefs){.fg_color = TERM_COLOR_RED, .mode.bold = true}, "[KOVSH ERR]: "msg"\n", __VA_ARGS__))

#define STRV_LIT(lit) { sizeof(lit)-1, (lit) }
#define STRV_FMT "%.*s"
#define STRV_ARG(sv) (int) (sv).len, (sv).items

typedef enum {
    KSH_ERR_OK = 0,
    KSH_ERR_COMMAND_NOT_FOUND,
    KSH_ERR_ARG_NOT_FOUND,
    KSH_ERR_TOKEN_EXPECTED,
    KSH_ERR_TYPE_EXPECTED,
    KSH_ERR_ASSIGNMENT_EXPECTED,
    KSH_ERR_MEM_OVER,
    KSH_ERR_PATTERN_NOT_FOUND,
    KSH_ERR_VAR_NOT_FOUND
} KshErr;

typedef struct {
    size_t len;
    const char *items;
} StrView;

typedef enum {
    KSH_VALUE_TYPE_STR,
    KSH_VALUE_TYPE_INT,
    KSH_VALUE_TYPE_BOOL,
    KSH_VALUE_TYPE_ANY
} KshValueType;

typedef union {
    StrView as_str;
    int     as_int;
    bool    as_bool;
} KshValue;

const char *ksh_err_str(KshErr err);

StrView strv_from_str(const char *str);
StrView strv_new(const char *data, size_t data_len);
bool    strv_eq(StrView sv1, StrView sv2);

const char *ksh_val_type_str(KshValueType t);

///////////////////////////////////////////////
/// COMMAND
//////////////////////////////////////////////

typedef struct {
    StrView name;
    KshValueType type;
    KshValue default_val;
    const char *usage;
    bool has_default;
} ArgDef;

typedef struct {
    ArgDef *def;
    KshValue value;
    bool is_assign;
} Arg;

typedef int (*CommandFn)(KshValue *args);

typedef struct {
    StrView name;
    const char *desc;
    CommandFn fn;
    ArgDef *arg_defs;
    size_t arg_defs_len;
} Command;

typedef struct {
    Command *cmd;
    Arg     *argv;
    size_t  argc;
    size_t  last_assigned_arg_idx;
} CommandCall;

typedef struct {
    Command *items;
    size_t len;
} CommandBuf;

void ksh_cmd_print(Command cmd);
Command *ksh_cmd_find_local(CommandBuf buf, StrView sv);
Command *ksh_cmd_find_hardcoded(StrView sv);
Command *ksh_cmd_find(CommandBuf buf, StrView sv);
CommandCall ksh_cmd_create_call(Command *cmd);
ArgDef *ksh_cmd_find_arg_def(Command *cmd, StrView name);
KshErr ksh_cmd_get(CommandBuf buf, StrView sv, Command *out);

KshErr ksh_cmd_call_exec(CommandCall call);

void ksh_arg_def_print(ArgDef cmd_arg);

Arg *ksh_args_find(size_t argc, Arg argv[argc], StrView sv);


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
    bool overrides_global;
} PromptPart;

typedef struct {
    PromptPart   *parts;
    size_t        parts_len;
    TermTextPrefs global_prefs;
} Prompt;

typedef enum {
    TERMINAL_MOD_DEF,
    TERMINAL_MOD_SYS,
} TerminalMod;

typedef struct {
    CommandBuf  commands;
    Prompt      prompt;
    TerminalMod mod;
    CommandCall cur_cmd_call;
    bool        should_exit;
} Terminal;

typedef struct {
    StrView name;
    struct {
        char  *items;
        size_t len;
    } value;
    bool    is_mutable;
} Variable;

KshErr ksh_var_add(Variable var);
KshErr ksh_var_get_val(StrView name, StrView *dest);
KshErr ksh_var_set_val(StrView name, StrView value);

void ksh_term_add_command(Command cmd);
void ksh_term_set_prompt(Prompt p);
void ksh_term_set_mod(TerminalMod mod);
void ksh_term_run(void);

void ksh_prompt_print(Prompt p);

void ksh_termio_init(void);
void ksh_termio_print(TermTextPrefs prefs, const char *fmt, ...);
void ksh_termio_getline(size_t size, char buf[size]);
void ksh_termio_end(void);

///////////////////////////////////////////////
/// LEXER
//////////////////////////////////////////////

typedef enum {
    TOKEN_TYPE_LIT,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_VAR,
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

Lexer ksh_lexer_new(StrView sv);
const char *ksh_lexer_token_type_to_string(TokenType token_type);
bool ksh_lexer_peek_token(Lexer *l, Token *t);
bool ksh_lexer_next_token(Lexer *l, Token *t);
bool ksh_lexer_is_next_token(Lexer *l, TokenType tt);
bool ksh_lexer_next_token_if(Lexer *l, TokenType tt, Token *t);
KshErr ksh_lexer_expect_next_token(Lexer *l, TokenType expect, Token *out);

KshErr ksh_token_parse_to_value(Token tok, KshValueType type, KshValue *dest);
KshErr ksh_token_from_strv(StrView sv, Token *dest);
KshErr ksh_token_type_to_value(TokenType t, KshValueType *dest);

KshErr ksh_parse(Lexer *lex, Terminal *term);

#endif
