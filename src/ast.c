#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "utils.h"

AST *ast_alloc(void)
{
    AST *ast = malloc(sizeof(AST));

    if (ast == NULL) {
        ALLOCATION_ERROR();
    }

    ast->data_type = data_type_type(TYPE_NULL);

    return ast;
}

void ast_print(const AST *ast)
{
    switch (ast->type) {
        case AST_NODE: {
            printf("%.*s", (int) ast->node.len, ast->node.text);
            break;
        }

        case AST_INFIX: {
            printf("(");
            ast_print(ast->infix.lhs);
            printf(" %.*s ", (int) ast->infix.oper.len, ast->infix.oper.text);
            ast_print(ast->infix.rhs);
            printf(")");
            break;
        }

        case AST_PREFIX: {
            printf("(");
            printf("%.*s", (int) ast->prefix.oper.len, ast->prefix.oper.text);
            ast_print(ast->prefix.node);
            printf(")");
            break;
        }

        case AST_BLOCK: {
            printf("{ ");
            for (size_t i = 0; i < ast->block.len; ++i) {
                ast_print(ast->block.statements[i]);
                printf("; ");
            }
            printf("}");
            break;
        }

        case AST_IF_STATEMENT: {
            printf("if ");
            ast_print(ast->if_statement.condition);
            printf(" ");
            ast_print(ast->if_statement.if_branch);
            if (ast->if_statement.else_branch != NULL) {
                printf(" else ");
                ast_print(ast->if_statement.else_branch);
            }
            break;
        }

        case AST_WHILE_LOOP: {
            printf("while ");
            ast_print(ast->while_loop.condition);
            printf(" ");
            ast_print(ast->while_loop.body);
            break;
        }

        case AST_DECLARATION: {
            printf("(");
            printf("%.*s: ", (int) ast->declaration.name.len, ast->declaration.name.text);
            ast_print(ast->declaration.type);
            printf(" = ");
            ast_print(ast->declaration.value);
            printf(")");
            break;
        }
    }
}

void ast_free(AST *ast)
{
    switch (ast->type) {
        case AST_NODE: {
            break;
        }

        case AST_INFIX: {
            ast_free(ast->infix.lhs);
            ast_free(ast->infix.rhs);
            break;
        }

        case AST_PREFIX: {
            ast_free(ast->prefix.node);
            break;
        }

        case AST_BLOCK: {
            for (size_t i = 0; i < ast->block.len; ++i) {
                ast_free(ast->block.statements[i]);
            }
            free(ast->block.statements);
            break;
        }

        case AST_IF_STATEMENT: {
            ast_free(ast->if_statement.condition);
            ast_free(ast->if_statement.if_branch);
            if (ast->if_statement.else_branch != NULL) {
                ast_free(ast->if_statement.else_branch);
            }
            break;
        }

        case AST_WHILE_LOOP: {
            ast_free(ast->while_loop.condition);
            ast_free(ast->while_loop.body);
            break;
        }

        case AST_DECLARATION: {
            ast_free(ast->declaration.type);
            if (ast->declaration.value != NULL) {
                ast_free(ast->declaration.value);
            }
            break;
        }
    }

    data_type_free(ast->data_type);

    free(ast);
}
