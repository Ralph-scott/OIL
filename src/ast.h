#ifndef AST_H_
#define AST_H_

#include "lexer.h"
#include "types.h"

typedef enum ASTType
{
    AST_NODE,
    AST_INFIX,
    AST_PREFIX,
    AST_BLOCK,
    AST_IF_STATEMENT,
    AST_WHILE_LOOP,
    AST_DECLARATION
} ASTType;

struct AST;
typedef struct AST AST;

typedef struct ASTInfix
{
    Token oper;
    AST *lhs;
    AST *rhs;
} ASTInfix;

typedef struct ASTPrefix
{
    Token oper;
    AST *node;
} ASTPrefix;

typedef struct ASTBlock
{
    size_t len;
    size_t cap;
    AST **statements;
} ASTBlock;

typedef struct ASTIfStatement
{
    AST *condition;
    AST *if_branch;
    AST *else_branch;
} ASTIfStatement;

typedef struct ASTWhileLoop
{
    AST *condition;
    AST *body;
} ASTWhileLoop;

typedef struct ASTDeclaration
{
    Token name;
    AST *type;
    AST *value;
} ASTDeclaration;

typedef struct AST
{
    ASTType type;
    DataType *data_type;
    union {
        Token node;
        ASTInfix infix;
        ASTPrefix prefix;
        ASTBlock block;
        ASTIfStatement if_statement;
        ASTWhileLoop while_loop;
        ASTDeclaration declaration;
    };
} AST;

AST *ast_alloc(void);
void ast_print(const AST *ast);
void ast_free(AST *ast);

#endif // AST_H_
