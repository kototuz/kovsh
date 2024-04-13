#include "cmd.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>


static Cmd *find_cmd_by_name(CmdList cmd_list, char *data, size_t data_len) {
    for (size_t i = 0; i < cmd_list.len; i++) {
        if (strncmp(cmd_list.items[i].name, data, data_len) == 0) {
            return &cmd_list.items[i];
        }
    }

    return NULL;
}

static CmdArg *find_cmd_arg_by_name(CmdArgList cmd_arg_list, char *data, size_t data_len) {
    for (size_t i = 0; i < cmd_arg_list.len; i++) {
        if (strncmp(cmd_arg_list.items[i].name, data, data_len) == 0) {
            return &cmd_arg_list.items[i];
        }
    }

    return NULL;
}

typedef struct {
    char *data;
    size_t data_len;
    char *cursor;
} Lexer;

typedef enum {
    TOKEN_DELIM_BEG,
    TOKEN_DELIM_END,
    TOKEN_DELIM_BEG_END,
    TOKEN_SYMBOL,
    TOKEN_EQ,
    TOKEN_END,
    TOKEN_INVALID,
} TokenType;

typedef struct {
    TokenType type;
    char *data;
    size_t data_len;
} Token;

static Lexer lexer_new(char *data, size_t data_len) {
    return (Lexer) {
        .data = data,
        .data_len = data_len,
        .cursor = data
    };
}

static void lexer_trim(Lexer *self) {
    while (*self->cursor == ' ') {
        self->cursor++;
    }
}

static const char *lexer_token_type_to_string(TokenType token_type) {
    switch (token_type) {
        case TOKEN_SYMBOL: return "symbol";
        case TOKEN_DELIM_BEG: return "delim_beg";
        case TOKEN_DELIM_END: return "delim_end";
        case TOKEN_DELIM_BEG_END: return "delim_beg_end";
        case TOKEN_EQ: return "equal";
        case TOKEN_END: return "end";
        default: return "invalid";
    }
}

static TokenType lexer_token_type(int letter) {
    switch (letter) {
        case '\'':
        case '"':
            return TOKEN_DELIM_BEG_END;

        case '(':
        case '[':
        case '{':
        case '<':
            return TOKEN_DELIM_BEG;

        case ')':
        case ']':
        case '}':
        case '>':
            return TOKEN_DELIM_END;

        case '=':
            return TOKEN_EQ;

        case '\0':
            return TOKEN_END;
    }
    return TOKEN_SYMBOL;
}

static bool is_symbol(int letter) {
    return isalpha(letter);
}

typedef struct TokenNode {
    Token val;
    struct TokenNode *next;
} TokenNode;

typedef struct {
    TokenNode *begin;
    TokenNode *end;
    size_t len;
} TokenSeq;

static TokenSeq lexer_tokenize(Lexer *self) {
    TokenSeq result = {0};

    TokenNode *token_node = &(TokenNode){0};
    TokenNode **result_begin = &token_node->next;

    TokenType token_type = lexer_token_type(*self->cursor);
    lexer_trim(self);
    while (token_type != TOKEN_END) {
        token_node = token_node->next = malloc(sizeof *token_node);
        if (token_node == NULL) {
            fputs("[!] lexer_tokenize: could not allocate memory", stderr);
            return result;
        }

        token_node->val = (Token) {
            .type = token_type,
            .data = self->cursor,
            .data_len = 1,
        };

        if (token_type == TOKEN_SYMBOL) {
            self->cursor++;
            while (is_symbol(*self->cursor)) {
                self->cursor++;
                token_node->val.data_len++;
            }
        } else {
            self->cursor++;
        }

        lexer_trim(self);
        token_type = lexer_token_type(*self->cursor);
        result.len++;
    }

    result.begin = *result_begin;
    result.end = token_node;

    return result;
}

static Token lexer_next_token(Lexer *self) {
    lexer_trim(self);

    Token token = {0};
    token.type = lexer_token_type(*self->cursor);
    token.data = self->cursor;
    token.data_len = 1;

    if (token.type == TOKEN_SYMBOL) {
        self->cursor++;
        while (is_symbol(*self->cursor)) {
            self->cursor++;
            token.data_len++;
        }
        return token;
    }

    self->cursor++;

    return token;
}

typedef enum {
    ANALYZE_START,
    ANALYZE_CMD_FOUND,
    ANALYZE_ARG_FOUND,
} AnalyzeState;

typedef struct {
    void *ptr;
    AnalyzeState state;
} Analyzer;

static void analyzer_deter(Token *token, Analyzer analyzer) {
}

static void parse_tokens(Lexer *l) {
    Analyzer analyzer = {.ptr = NULL, .state = ANALYZE_START};
    Token token = lexer_next_token(l);
    while (token.type != TOKEN_END) {
        if (token.type == TOKEN_SYMBOL) {
        }
    }
}

int main(void) {
    char *text = "    echo    msg   =   'Hello, world'";
    Lexer lexer = lexer_new(text, strlen(text));

    TokenSeq token_seq = lexer_tokenize(&lexer);

    TokenNode *node = token_seq.begin;
    for (size_t i = 0; i < token_seq.len; i++) {
        for (size_t y = 0; y < node->val.data_len; y++) {
            putchar(node->val.data[y]);
        }
        printf("\t[%s]\n", lexer_token_type_to_string(node->val.type));

        node = node->next;
    }

    return 0;
}
//
//#define PAIRS_LEN 6
//const char pairSymbols[] = {
//    '"', '"',
//    '\'', '\'',
//    '(', ')',
//    '[', ']',
//    '{', '}',
//    '<', '>'
//};
//
//static int nextCmdArgToken(CmdArgToken *token, char **string) {
//    char *strCopy = *string;
//    while (*strCopy == ' ') {
//        strCopy++;
//    }
//
//    if (*strCopy == '\0') return 0;
//
//    char *tokenArgNamePtr = strCopy;
//
//    char *tokenAssignOperatorPtr = strchr(tokenArgNamePtr, '=');
//    if (tokenAssignOperatorPtr == NULL) {
//        fputs("ERROR: the assign operator `=` was missed", stderr);
//        return -1;
//    }
//
//    *tokenAssignOperatorPtr = '\0';
//    *(tokenArgNamePtr + strcspn(tokenArgNamePtr, " ")) = '\0';
//
//    tokenAssignOperatorPtr++;
//    while (*tokenAssignOperatorPtr == ' ') {
//        tokenAssignOperatorPtr++;
//    }
//
//    if (*tokenAssignOperatorPtr == '\0') {
//        fputs("ERROR: the arg was missed", stderr);
//        return -1;
//    }
//
//    char *tokenLastSymbol;
//    for (size_t i = 0, y = 0; i < PAIRS_LEN; i++, y += 2) {
//        if (*tokenAssignOperatorPtr == pairSymbols[y]) {
//            tokenAssignOperatorPtr++;
//            tokenLastSymbol = strchr(tokenAssignOperatorPtr, pairSymbols[y+1]);
//            if (tokenLastSymbol == NULL) {
//                fputs("ERROR: the end pair symbol is missed", stderr);
//                return -1;
//            }
//            goto ok;
//        }
//    }
//
//    tokenLastSymbol = tokenAssignOperatorPtr + strcspn(tokenAssignOperatorPtr, " ");
//
//ok:
//    *tokenLastSymbol = '\0';
//    *string = tokenLastSymbol + 1;
//
//    token->name = tokenArgNamePtr;
//    token->valString = tokenAssignOperatorPtr;
//
//    return 1;
//}
//
//static int cmdParseString(Cmdline *cmdline, char *string) {
//    Cmd *cmd = findCmdByName(strtok(string, " "));
//}
//
//// TEST
//int main(void) {
//    char str[] = "arg1='Hello, World' arg2={Maybe} arg3 =     'What is it?'";
//    char *str_p = str;
//
//    CmdArgToken token;
//    while (nextCmdArgToken(&token, &str_p)) {
//        printf("token = [name: %s, valString: %s]\n", token.name, token.valString);
//    }
//
//    return 0;
//}
//
////static void getCmdArgValueBorders(CmdArgParseInfo parseInfo,
////                                  char *beginString,
////                                  char **outBegin,
////                                  char **outEnd) {
////    *outBegin = parseInfo.beginSign == 0 ?
////                beginString :
////                strchr(beginString, parseInfo.beginSign);
////
////    if (*outBegin == NULL) return;
////
////    char *valueEnd;
////    if (parseInfo.endSign == 0) {
////        valueEnd = *outBegin;
////        while (*valueEnd != ' ' && *valueEnd != '\0') {
////            valueEnd++;
////        }
////    } else {
////        valueEnd = strchr(*outBegin, parseInfo.endSign);
////    }
////
////    *outEnd = valueEnd;
////}
//
////
////static int parseCmdArgs(Cmd *cmd, char *argsString) {
////    for (;;) {
////        if (*argsString == '\0') break;
////
////        char *assignOperator = strchr(argsString, '=');
////        if (assignOperator == NULL) {
////            fprintf(stderr, "ERROR: the argument assignment operator `=` was expected\n");
////            return -1;
////        }
////
////        *assignOperator = '\0';
////        CmdArg *cmdArg = findCmdArgByName(cmd, argsString);
////        if (cmdArg == NULL) {
////            fprintf(stderr, "ERROR: the argument `%s` does not exist\n", argsString);
////            return -1;
////        }
////
////        char *argValueBegin;
////        char *argValueEnd;
////        getCmdArgValueBorders(cmdArg->parseInfo,
////                              ++assignOperator,
////                              &argValueBegin,
////                              &argValueEnd);
////
////        if (argValueBegin == NULL || argValueEnd == NULL) {
////            fprintf(stderr, "ERROR: the argument value borders were expected");
////            return -1;
////        }
////
////        cmdArg->parseInfo.parser(cmdArg, argValueBegin);
////
////        if (argValueEnd == ' ') *argValueEnd++ = '\0';
////        argsString = argValueEnd;
////    }
////}
////
////static int parseCmdlineString(const char *cmdline) {
////    Cmd *cmd = findCmdByName(strtok(cmdline, " "));
////    if (cmd == NULL) {
////        fprintf(stderr, "ERROR: the cmd does not exist");
////        return -1;
////    }
////
////    if (!parseCmdArgs(cmd, cmdline)) return -1;
////}
