#ifndef LEXER_H_
#define LEXER_H_

#include <stddef.h>

typedef enum TokenType
{
    TOKEN_IDENT,
    TOKEN_NUMBER,

    TOKEN_IF,
    TOKEN_ELSE,

    TOKEN_WHILE,

    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,

    TOKEN_LEFT_CURLY,
    TOKEN_RIGHT_CURLY,

    TOKEN_OPER_ADD,
    TOKEN_OPER_SUB,
    TOKEN_OPER_MUL,
    TOKEN_OPER_DIV,
    TOKEN_OPER_LT,
    TOKEN_OPER_GT,
    TOKEN_OPER_LT_OR_EQUALS,
    TOKEN_OPER_GT_OR_EQUALS,
    TOKEN_OPER_NOT_EQUALS,
    TOKEN_OPER_EQUALS,

    TOKEN_NOT,

    TOKEN_ASSIGN,

    TOKEN_COLON,
    TOKEN_SEMICOLON,

    TOKEN_INVALID,

    TOKEN_EOF,

    TOKEN_TYPES
} TokenType;

typedef struct StringTokenTypeTuple
{
    char *text;
    TokenType type;
} StringTokenTypeTuple;

static const StringTokenTypeTuple STRING_TO_TOKEN_TYPE[] = {
    {"if",    TOKEN_IF},
    {"else",  TOKEN_ELSE},
    {"while", TOKEN_WHILE}
};

static const char *TOKEN_TYPE_TO_STRING[TOKEN_TYPES] = {
    [TOKEN_IDENT]             = "IDENT",
    [TOKEN_NUMBER]            = "NUMBER",
    [TOKEN_IF]                = "IF",
    [TOKEN_ELSE]              = "ELSE",
    [TOKEN_WHILE]             = "WHILE",
    [TOKEN_LEFT_PAREN]        = "LEFT_PAREN",
    [TOKEN_RIGHT_PAREN]       = "RIGHT_PAREN",
    [TOKEN_LEFT_CURLY]        = "LEFT_CURLY",
    [TOKEN_RIGHT_CURLY]       = "RIGHT_CURLY",
    [TOKEN_OPER_ADD]          = "OPER_ADD",
    [TOKEN_OPER_SUB]          = "OPER_SUB",
    [TOKEN_OPER_MUL]          = "OPER_MUL",
    [TOKEN_OPER_DIV]          = "OPER_DIV",
    [TOKEN_OPER_LT]           = "OPER_LT",
    [TOKEN_OPER_GT]           = "OPER_GT",
    [TOKEN_OPER_LT_OR_EQUALS] = "OPER_LT_OR_EQUALS",
    [TOKEN_OPER_GT_OR_EQUALS] = "OPER_GT_OR_EQUALS",
    [TOKEN_OPER_NOT_EQUALS]   = "OPER_NOT_EQUALS",
    [TOKEN_OPER_EQUALS]       = "OPER_EQUALS",
    [TOKEN_NOT]               = "NOT",
    [TOKEN_ASSIGN]            = "ASSIGN",
    [TOKEN_COLON]             = "COLON",
    [TOKEN_SEMICOLON]         = "SEMICOLON",
    [TOKEN_INVALID]           = "INVALID",
    [TOKEN_EOF]               = "EOF"
};

typedef struct Token
{
    TokenType type;
    char *text;
    size_t pos;
    size_t len;
} Token;

typedef struct Lexer
{
    size_t len;
    char *text;
    size_t pos;
} Lexer;

Lexer lexer_new(char *text);
Token lexer_next(Lexer *lexer);
Token lexer_peek(Lexer *lexer);

#endif // LEXER_H_ 
