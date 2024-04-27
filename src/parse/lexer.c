#include "../kovsh.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

// KEYWORDS
const StrSlice BOOL_TRUE_KEYWORD = { .items = "true", .len = 4 };
const StrSlice BOOL_FALSE_KEYWORD = { .items = "false", .len = 5 };

// PRIVATE FUNCTION DECLARATIONS
static void lexer_trim(Lexer *l);
static bool is_lit(int letter);
static bool lexer_at_keyword(Lexer *l, StrSlice keyword);



// PUBLIC FUNCTIONS
Lexer lexer_new(StrSlice ss)
{
    return (Lexer) { .text = ss };
}

const char *lexer_token_type_to_string(TokenType token_type)
{
    switch (token_type) {
    case TOKEN_TYPE_LIT: return "literal";
    case TOKEN_TYPE_STRING: return "string";
    case TOKEN_TYPE_NUMBER: return "number";
    case TOKEN_TYPE_BOOL: return "bool";
    case TOKEN_TYPE_EQ: return "equal";
    case TOKEN_TYPE_END: return "end";
    default: return "invalid";
    }
}

//TokenSeq lexer_tokenize(Lexer *self) {
//    TokenSeq result = {0};
//
//    TokenNode *token_node = &(TokenNode){0};
//    TokenNode **result_begin = &token_node->next;
//
//    TokenType token_type = lexer_token_type(*self->cursor);
//    lexer_trim(self);
//    while (token_type != TOKEN_TYPE_END) {
//        token_node = token_node->next = malloc(sizeof *token_node);
//        if (token_node == NULL) {
//            fputs("[!] lexer_tokenize: could not allocate memory", stderr);
//            return result;
//        }
//
//        token_node->val = (Token) {
//            .type = token_type,
//            .data = self->cursor,
//            .data_len = 1,
//        };
//
//        if (token_type == TOKEN_SYMBOL) {
//            self->cursor++;
//            while (lexer_is_symbol(*self->cursor)) {
//                self->cursor++;
//                token_node->val.data_len++;
//            }
//        } else {
//            self->cursor++;
//        }
//
//        lexer_trim(self);
//        token_type = lexer_token_type(*self->cursor);
//        result.len++;
//    }
//
//    result.begin = *result_begin;
//    result.end = token_node;
//
//    return result;
//}

Token lexer_token_next(Lexer *l)
{
    Token result = {0};

    lexer_trim(l);
    size_t cursor = l->cursor;

    int first_letter = l->text.items[cursor];
    switch (first_letter) {
        case '=':
            result.type = TOKEN_TYPE_EQ;
            result.text.items = &l->text.items[cursor++];
            result.text.len = 1;
            break;

        case '"':
            result.type = TOKEN_TYPE_STRING;
            result.text.items = &l->text.items[++cursor];
            while (l->text.items[cursor] != '"') {
                if (l->text.items[cursor] == '\0') {
                    fputs("ERROR: the closing quote was missed\n", stderr);
                    exit(1);
                }

                result.text.len++;
                cursor++;
            }
            cursor++;
            break;

        case '\0':
            result.type = TOKEN_TYPE_END;
            break;

        default:
            l->cursor = cursor;
            if (lexer_at_keyword(l, BOOL_FALSE_KEYWORD)) {
                result.type = TOKEN_TYPE_BOOL;
                result.text = BOOL_FALSE_KEYWORD;

                cursor += BOOL_FALSE_KEYWORD.len;
            } else if (lexer_at_keyword(l, BOOL_TRUE_KEYWORD)) {
                result.type = TOKEN_TYPE_BOOL;
                result.text = BOOL_TRUE_KEYWORD;

                cursor += BOOL_TRUE_KEYWORD.len;
            } else if (isdigit(l->text.items[cursor])) {
                result.type = TOKEN_TYPE_NUMBER;
                result.text.items = &l->text.items[cursor++];
                result.text.len = 1;
                while (isdigit(l->text.items[cursor])) {
                    cursor++;
                    result.text.len++;
                }
            } else if (is_lit(l->text.items[cursor])) {
                result.type = TOKEN_TYPE_LIT;
                result.text.items = &l->text.items[cursor++];
                result.text.len = 1;
                while (is_lit(l->text.items[cursor])) {
                    cursor++;
                    result.text.len++;
                }
            }

            break;
    }

    l->cursor = cursor;

    return result;
}

void lexer_inc(Lexer *l, size_t inc)
{
    l->cur_letter = l->text.items[(l->cursor += inc)];
}

void lexer_expect_token_next(Lexer *l, TokenType expect)
{
    Token t = lexer_token_next(l);
    if (t.type != expect) {
        printf("ERROR: %s was expected, but %s is occured\n",
               lexer_token_type_to_string(expect),
               lexer_token_type_to_string(t.type));
    }
}

int main(void)
{
    const char *text = "echo msg=\"Hello world\" rep=123 up=true";
    StrSlice ss = { .items = text, .len = strlen(text) };

    Lexer lexer = lexer_new(ss);

    Token t = lexer_token_next(&lexer);
    while (t.type != TOKEN_TYPE_END) {
        for (size_t i = 0; i < t.text.len; i++) {
            putchar(t.text.items[i]);
        }
        printf(" -> [%s]\n", lexer_token_type_to_string(t.type));

        t = lexer_token_next(&lexer);
    }
}

// PRIVATE FUNCTIONS
//static Cmd *find_cmd_by_name(CmdList cmd_list, char *data, size_t data_len) {
//    for (size_t i = 0; i < cmd_list.len; i++) {
//        if (strncmp(cmd_list.items[i].name, data, data_len) == 0) {
//            return &cmd_list.items[i];
//        }
//    }
//
//    return NULL;
//}
//
//static CmdArg *find_cmd_arg_by_name(CmdArgList cmd_arg_list, char *data, size_t data_len) {
//    for (size_t i = 0; i < cmd_arg_list.len; i++) {
//        if (strncmp(cmd_arg_list.items[i].name, data, data_len) == 0) {
//            return &cmd_arg_list.items[i];
//        }
//    }
//
//    return NULL;
//}

static void lexer_trim(Lexer *self) {
    while (self->text.items[self->cursor] == ' ') {
        self->cursor++;
    }
}

static bool is_lit(int letter)
{
    return ('a' <= letter && letter <= 'z')
           || ('0' <= letter && letter <= '9');
}

static bool lexer_at_keyword(Lexer *l, StrSlice ss)
{
    if ((l->text.len - l->cursor == ss.len)
       || isspace(l->text.items[l->cursor+ss.len])) {
        return strncmp(&l->text.items[l->cursor], ss.items, ss.len) == 0;
    }

    return false;
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
