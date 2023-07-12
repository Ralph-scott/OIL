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

void ast_print(FILE *file, const AST *ast)
{
    switch (ast->type) {
        case AST_NODE: {
            fprintf(file, "%.*s", (int) ast->node.len, ast->node.text);
            break;
        }

        case AST_INFIX: {
            fprintf(file, "(");
            ast_print(file, ast->infix.lhs);
            fprintf(file, " %.*s ", (int) ast->infix.oper.len, ast->infix.oper.text);
            ast_print(file, ast->infix.rhs);
            fprintf(file, ")");
            break;
        }

        case AST_PREFIX: {
            fprintf(file, "(");
            fprintf(file, "%.*s ", (int) ast->prefix.oper.len, ast->prefix.oper.text);
            ast_print(file, ast->prefix.node);
            fprintf(file, ")");
            break;
        }

        case AST_BLOCK: {
            fprintf(file, "{ ");
            for (size_t i = 0; i < ast->block.len; ++i) {
                ast_print(file, ast->block.statements[i]);
                if (i + 1 < ast->block.len) {
                    fprintf(file, "; ");
                }
            }
            fprintf(file, "}");
            break;
        }

        case AST_FUNCTION_CALL: {
            ast_print(file, ast->function_call.lhs);
            fprintf(file, "(");
            for (size_t i = 0; i < ast->function_call.len; ++i) {
                ast_print(file, ast->function_call.arguments[i]);
                if (i + 1 < ast->function_call.len) {
                    fprintf(file, ", ");
                }
            }
            fprintf(file, ")");
            break;
        }

        case AST_IF_STATEMENT: {
            fprintf(file, "if ");
            ast_print(file, ast->if_statement.condition);
            fprintf(file, " ");
            ast_print(file, ast->if_statement.if_branch);
            if (ast->if_statement.else_branch != NULL) {
                fprintf(file, " else ");
                ast_print(file, ast->if_statement.else_branch);
            }
            break;
        }

        case AST_WHILE_LOOP: {
            fprintf(file, "while ");
            ast_print(file, ast->while_loop.condition);
            fprintf(file, " ");
            ast_print(file, ast->while_loop.body);
            break;
        }

        case AST_DECLARATION: {
            fprintf(file, "(");
            fprintf(file, "%.*s: ", (int) ast->declaration.name.len, ast->declaration.name.text);
            ast_print(file, ast->declaration.type);
            fprintf(file, " = ");
            ast_print(file, ast->declaration.value);
            fprintf(file, ")");
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

        case AST_FUNCTION_CALL: {
            ast_free(ast->function_call.lhs);
            for (size_t i = 0; i < ast->function_call.len; ++i) {
                ast_free(ast->function_call.arguments[i]);
            }
            free(ast->function_call.arguments);
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
