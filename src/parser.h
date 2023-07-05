#ifndef PARSER_H_
#define PARSER_H_

#include <stdbool.h>
#include "ast.h"
#include "lexer.h"

static const bool IS_PREFIX[TOKEN_TYPES] = {
    [TOKEN_OPER_SUB]  = true,
    [TOKEN_NOT]       = true,
    [TOKEN_REFERENCE] = true,
    [TOKEN_DEREFERENCE] = true
};

static const bool IS_NODE[TOKEN_TYPES] = {
    [TOKEN_IDENT]  = true,
    [TOKEN_NUMBER] = true,
    [TOKEN_STRING] = true
};

static const bool IS_END[TOKEN_TYPES] = {
    [TOKEN_EOF]         = true,
    [TOKEN_RIGHT_PAREN] = true,
    [TOKEN_RIGHT_CURLY] = true
};

AST *parse(Lexer *lexer);

#endif // PARSER_H_
