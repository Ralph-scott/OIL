#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lexer.h"
#include "utils.h"

static inline bool is_ident(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_';
}

static inline bool is_digit(char ch)
{
    return ch >= '0' && ch <= '9';
}

static inline bool is_whitespace(char ch)
{
    return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
}

Lexer lexer_new(char *text)
{
    Lexer lexer = {
        .len = strlen(text),
        .text = text,
        .pos = 0
    };

    return lexer;
}

Token lexer_next(Lexer *lexer)
{
    loop {
        while (is_whitespace(lexer->text[lexer->pos])) {
            ++lexer->pos;
        }

        const size_t start = lexer->pos;

        TokenType type;
        switch (lexer->text[lexer->pos]) {
            case '\0': {
                type = TOKEN_EOF;
                break;
            }

            case '"': {
                type = TOKEN_STRING;
                ++lexer->pos;
                while (lexer->text[lexer->pos] != '"') {
                    if (lexer->text[lexer->pos] == '\0') {
                        ERROR("String literal not closed off.");
                    }
                    if (lexer->text[lexer->pos] == '\\') {
                        ++lexer->pos;
                        if (lexer->text[lexer->pos] == '\0') {
                            ERROR("Cannot escape an EOF.");
                        }
                    }
                    ++lexer->pos;
                }
                ++lexer->pos;
                break;
            }

            case 'a' ... 'z':
            case 'A' ... 'Z':
            case '_': {
                type = TOKEN_IDENT;

                while (is_ident(lexer->text[lexer->pos])) {
                    ++lexer->pos;
                }

                for (size_t i = 0; i < ARRAY_LEN(STRING_TO_TOKEN_TYPE); ++i) {
                    const char *text = STRING_TO_TOKEN_TYPE[i].text;
                    if (strncmp(text, lexer->text + start, strlen(text)) == 0) {
                        type = STRING_TO_TOKEN_TYPE[i].type;
                    }
                }

                break;
            }

            case '0' ... '9': {
                type = TOKEN_NUMBER;

                while (is_digit(lexer->text[lexer->pos])) {
                    ++lexer->pos;
                }

                break;
            }


            case '(': { type = TOKEN_LEFT_PAREN; ++lexer->pos; break; }
            case ')': { type = TOKEN_RIGHT_PAREN; ++lexer->pos; break; }

            case '{': { type = TOKEN_LEFT_CURLY; ++lexer->pos; break; }
            case '}': { type = TOKEN_RIGHT_CURLY; ++lexer->pos; break; }

            case '#': { type = TOKEN_REFERENCE; ++lexer->pos; break; }
            case '@': { type = TOKEN_DEREFERENCE; ++lexer->pos; break; }

            case '+': { type = TOKEN_OPER_ADD; ++lexer->pos; break; }
            case '-': { type = TOKEN_OPER_SUB; ++lexer->pos; break; }
            case '*': { type = TOKEN_OPER_MUL; ++lexer->pos; break; }

            case '/': {
                type = TOKEN_OPER_DIV;
                ++lexer->pos;
                if (lexer->text[lexer->pos] == '/') {
                    while (lexer->text[lexer->pos] != '\n') {
                        ++lexer->pos;
                    }
                    ++lexer->pos;
                    continue;
                }
                break;
            }

            case '<': {
                type = TOKEN_OPER_LT;
                ++lexer->pos;
                if (lexer->text[lexer->pos] == '=') {
                    ++lexer->pos;
                    type = TOKEN_OPER_LT_OR_EQUALS;
                }
                break;
            }

            case '>': {
                type = TOKEN_OPER_GT;
                ++lexer->pos;
                if (lexer->text[lexer->pos] == '=') {
                    ++lexer->pos;
                    type = TOKEN_OPER_GT_OR_EQUALS;
                }
                break;
            }

            case '!': {
                type = TOKEN_NOT;
                ++lexer->pos;
                if (lexer->text[lexer->pos] == '=') {
                    ++lexer->pos;
                    type = TOKEN_OPER_NOT_EQUALS;
                }
                break;
            }

            case '=': {
                type = TOKEN_ASSIGN;
                ++lexer->pos;
                if (lexer->text[lexer->pos] == '=') {
                    ++lexer->pos;
                    type = TOKEN_OPER_EQUALS;
                }
                break;
            }

            case ':': { type = TOKEN_COLON; ++lexer->pos; break; }
            case ';': { type = TOKEN_SEMICOLON; ++lexer->pos; break; }

            default: {
                type = TOKEN_INVALID;

                ++lexer->pos;

                break;
            }
        }

        return (Token) {
            .type = type,
            .text = lexer->text + start,
            .pos = start,
            .len = lexer->pos - start
        };
    }
}

Token lexer_peek(Lexer *lexer)
{
    const Token token = lexer_next(lexer);
    lexer->pos = token.pos;
    return token;
}
