#define _GNU_SOURCE // needed for asprintf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compile.h"
#include "parser.h"
#include "asm_context.h"
#include "type_checker.h"
#include "types.h"
#include "utils.h"

AsmData compile_ast(Compiler *compiler, AST *ast);

static AsmData compile_node(Compiler *compiler, AST *node)
{
    switch (node->node.type) {
        case TOKEN_IDENT: {
            Variable variable = symbol_table_variable(&compiler->table, node->node.text, node->node.len);

            return asm_data_stack(variable.id * 8, variable.data_type);
        }

        case TOKEN_NUMBER: {
            AsmData asm_register = asm_context_data_alloc(&compiler->asm_context, node->data_type);

            size_t num;
            if (sscanf(node->node.text, "%zu", &num) == -1) {
                ERROR("Could not parse integer.");
            }

            asm_context_mov_constant(&compiler->asm_context, asm_register, num);

            return asm_register;
        }

        default: {
            UNREACHABLE();
        }
    }
}

static AsmData compile_infix(Compiler *compiler, AST *infix)
{
    AsmData lhs = compile_ast(compiler, infix->infix.lhs);
    AsmData rhs = compile_ast(compiler, infix->infix.rhs);

    AsmData result = asm_context_data_alloc(&compiler->asm_context, lhs.data_type);

    switch (infix->infix.oper.type) {
        case TOKEN_OPER_ADD: {
            asm_context_mov(&compiler->asm_context, result, lhs);
            asm_context_add(&compiler->asm_context, result, rhs);
            break;
        }

        case TOKEN_OPER_SUB: {
            asm_context_mov(&compiler->asm_context, result, lhs);
            asm_context_sub(&compiler->asm_context, result, rhs);
            break;
        }

        case TOKEN_OPER_MUL: {
            asm_context_mov(&compiler->asm_context, result, lhs);
            asm_context_mul(&compiler->asm_context, result, rhs);
            break;
        }

        case TOKEN_OPER_DIV: {
            asm_context_mov(&compiler->asm_context, result, lhs);
            asm_context_div(&compiler->asm_context, result, rhs);
            break;
        }

        case TOKEN_OPER_EQUALS: {
            asm_context_cmp(&compiler->asm_context, lhs, rhs);
            asm_context_setz(&compiler->asm_context, result);
            break;
        }

        case TOKEN_OPER_NOT_EQUALS: {
            asm_context_cmp(&compiler->asm_context, lhs, rhs);
            asm_context_setnz(&compiler->asm_context, result);
            break;
        }

        case TOKEN_OPER_GT: {
            asm_context_cmp(&compiler->asm_context, lhs, rhs);
            asm_context_setg(&compiler->asm_context, result);
            break;
        }

        case TOKEN_OPER_LT: {
            asm_context_cmp(&compiler->asm_context, lhs, rhs);
            asm_context_setl(&compiler->asm_context, result);
            break;
        }

        case TOKEN_OPER_GT_OR_EQUALS: {
            asm_context_cmp(&compiler->asm_context, lhs, rhs);
            asm_context_setge(&compiler->asm_context, result);
            break;
        }

        case TOKEN_OPER_LT_OR_EQUALS: {
            asm_context_cmp(&compiler->asm_context, lhs, rhs);
            asm_context_setle(&compiler->asm_context, result);
            break;
        }

        case TOKEN_ASSIGN: {
            asm_context_mov(&compiler->asm_context, lhs, rhs);
            asm_context_mov(&compiler->asm_context, result, lhs);
            break;
        }

        default: {
            UNREACHABLE();
        }
    }

    asm_context_data_free(&compiler->asm_context, lhs);
    asm_context_data_free(&compiler->asm_context, rhs);

    return result;
}

static AsmData compile_prefix(Compiler *compiler, AST *ast)
{
    AsmData node = compile_ast(compiler, ast->prefix.node);

    switch (ast->prefix.oper.type) {
        case TOKEN_OPER_SUB: {
            asm_context_negate(&compiler->asm_context, node);

            return node;
        }

        case TOKEN_NOT: {
            asm_context_test(&compiler->asm_context, node, node);
            asm_context_setz(&compiler->asm_context, node);

            return node;
        }

        case TOKEN_REFERENCE: {
            AsmData reference = asm_context_data_alloc(&compiler->asm_context, ast->data_type);

            asm_context_reference(&compiler->asm_context, reference, node);

            asm_context_data_free(&compiler->asm_context, node);

            return reference;
        }

        case TOKEN_DEREFERENCE: {
            AsmData deref = asm_context_dereference(&compiler->asm_context, node);

            asm_context_data_free(&compiler->asm_context, node);

            return deref;
        }

        default: {
            UNREACHABLE();
        }
    }
}

static AsmData compile_block(Compiler *compiler, AST *ast)
{
    AsmData statement = asm_context_data_alloc(&compiler->asm_context, ast->data_type);

    symbol_table_begin_scope(&compiler->table);

    for (size_t i = 0; i < ast->block.len; ++i) {
        asm_context_data_free(&compiler->asm_context, statement);
        statement = compile_ast(compiler, ast->block.statements[i]);
    }

    symbol_table_end_scope(&compiler->table);

    return statement;
}

static AsmData compile_if_statement(Compiler *compiler, AST *ast)
{
    const size_t if_label  = asm_context_label_new(&compiler->asm_context);
    const size_t end_label = asm_context_label_new(&compiler->asm_context);

    AsmData condition = compile_ast(compiler, ast->if_statement.condition);

    AsmData result = asm_context_data_alloc(&compiler->asm_context, condition.data_type);

    asm_context_test(&compiler->asm_context, condition, condition);
    asm_context_jz(&compiler->asm_context, if_label);

    asm_context_data_free(&compiler->asm_context, condition);

    AsmData if_block = compile_ast(compiler, ast->if_statement.if_branch);

    asm_context_mov(&compiler->asm_context, result, if_block);

    if (ast->if_statement.else_branch != NULL) {
        asm_context_jmp(&compiler->asm_context, end_label);
    }

    asm_context_label(&compiler->asm_context, if_label);

    if (ast->if_statement.else_branch != NULL) {
        AsmData else_block = compile_ast(compiler, ast->if_statement.else_branch);

        asm_context_mov(&compiler->asm_context, result, else_block);

        asm_context_data_free(&compiler->asm_context, else_block);
    }

    asm_context_label(&compiler->asm_context, end_label);

    asm_context_data_free(&compiler->asm_context, if_block);
    asm_context_data_free(&compiler->asm_context, condition);

    return result;
}

static AsmData compile_while_loop(Compiler *compiler, AST *ast)
{
    const size_t start_label = asm_context_label_new(&compiler->asm_context);
    const size_t end_label   = asm_context_label_new(&compiler->asm_context);

    asm_context_label(&compiler->asm_context, start_label);

    AsmData condition = compile_ast(compiler, ast->while_loop.condition);

    asm_context_test(&compiler->asm_context, condition, condition);
    asm_context_jz(&compiler->asm_context, end_label);

    asm_context_data_free(&compiler->asm_context, condition);

    AsmData body = compile_ast(compiler, ast->while_loop.body);

    asm_context_jmp(&compiler->asm_context, start_label);
    asm_context_label(&compiler->asm_context, end_label);

    return body;
}

static AsmData compile_declaration(Compiler *compiler, AST *ast)
{
    Variable variable = symbol_table_variable(&compiler->table, ast->declaration.name.text, ast->declaration.name.len);

    AsmData asm_variable = asm_data_stack(variable.id * 8, variable.data_type);

    if (ast->declaration.value != NULL) {
        AsmData value = compile_ast(compiler, ast->declaration.value);

        asm_context_mov(&compiler->asm_context, asm_variable, value);

        asm_context_data_free(&compiler->asm_context, value);
    }

    return asm_variable;
}

AsmData compile_ast(Compiler *compiler, AST *ast)
{
    switch (ast->type) {
        case AST_NODE: {
            return compile_node(compiler, ast);
        }

        case AST_INFIX: {
            return compile_infix(compiler, ast);
        }

        case AST_PREFIX: {
            return compile_prefix(compiler, ast);
        }

        case AST_BLOCK: {
            return compile_block(compiler, ast);
        }

        case AST_IF_STATEMENT: {
            return compile_if_statement(compiler, ast);
        }

        case AST_WHILE_LOOP: {
            return compile_while_loop(compiler, ast);
        }

        case AST_DECLARATION: {
            return compile_declaration(compiler, ast);
        }
    }
}

void compile(AST *ast, FILE *file)
{
    Compiler compiler = {
        .table = symbol_table_new(ast),
        .asm_context = asm_context_new(file)
    };

    asm_context_extend_stack(&compiler.asm_context, 8 * compiler.table.variable_id);

    AsmData data = compile_ast(&compiler, ast);

    asm_context_mov(&compiler.asm_context, asm_data_register(REGISTER_RAX, data.data_type), data);

    asm_context_data_free(&compiler.asm_context, data);
    asm_context_free(&compiler.asm_context);
    symbol_table_free(&compiler.table);
}
