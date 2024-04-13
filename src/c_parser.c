#include "cmd.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>


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

typedef struct {
    size_t len;
    Token *items;
} TokenSeq;

typedef enum {
    COND_RANGE,
    COND_EQ,
} CondType;

typedef struct {
    CondType type;
} Cond;


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
    return isalnum(letter);
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

int main(void) {
    char *text = "     echo     msg  =  'Hello World'";
    Lexer lexer = lexer_new(text, strlen(text));

    Token token = lexer_next_token(&lexer);
    while (token.type != TOKEN_END) {
        for (size_t i = 0; i < token.data_len; i++) {
            putchar(token.data[i]);
        }
        putchar('\n');
        
        token = lexer_next_token(&lexer);
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
