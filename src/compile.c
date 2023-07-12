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

static char *string_literal_to_string(char *text, size_t text_len)
{
    size_t out_text_len = 0;
    char *out_text = malloc(sizeof(*out_text) * (text_len + 1));

    if (out_text == NULL) {
        ALLOCATION_ERROR();
    }

    for (size_t i = 1; i < text_len - 1; ++i) {
        if (text[i] != '\\') {
            out_text[out_text_len++] = text[i];
            continue;
        }

        ++i;
        switch (text[i]) {
            case 'r': {
                out_text[out_text_len++] = '\r';
                break;
            }

            case 'n': {
                out_text[out_text_len++] = '\n';
                break;
            }

            case 't': {
                out_text[out_text_len++] = '\t';
                break;
            }

            case '0': {
                out_text[out_text_len++] = '\0';
                break;
            }

            default: {
                out_text[out_text_len++] = text[i];
                break;
            }
        }
    }

    out_text[out_text_len] = '\0';

    return out_text;
}

AsmData compile_ast(Compiler *compiler, AST *ast);

static AsmData compile_node(Compiler *compiler, AST *ast)
{
    switch (ast->node.type) {
        case TOKEN_IDENT: {
            Variable variable      = symbol_table_variable(&compiler->table, ast->node.len, ast->node.text);
            VariableID variable_id = symbol_table_variable_id(&compiler->table, ast->node.len, ast->node.text);

            if (ast->data_type->type == TYPE_FUNCTION) {
                return asm_data_function(ast->node.len, ast->node.text, ast->data_type);
            }

            return asm_data_stack_variable(asm_context_variable_stack_position(&compiler->asm_context, variable_id), variable.data_type);
        }

        case TOKEN_NUMBER: {
            AsmData asm_register = asm_context_data_alloc(&compiler->asm_context, ast->data_type);

            size_t num;
            if (sscanf(ast->node.text, "%zu", &num) == -1) {
                ERROR("Could not parse integer.");
            }

            asm_context_mov_constant(&compiler->asm_context, asm_register, num);

            return asm_register;
        }

        case TOKEN_STRING: {
            char *literal = string_literal_to_string(ast->node.text, ast->node.len);
            return asm_context_add_to_data_section(&compiler->asm_context, literal, strlen(literal) + 1, ast->data_type);
        }

        default: {
            UNREACHABLE();
        }
    }
}

static AsmData compile_infix(Compiler *compiler, AST *ast)
{
    AsmData lhs = compile_ast(compiler, ast->infix.lhs);
    AsmData rhs = compile_ast(compiler, ast->infix.rhs);

    AsmData result = asm_context_data_alloc(&compiler->asm_context, lhs.data_type);

    switch (ast->infix.oper.type) {
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

static AsmData compile_function_call(Compiler *compiler, AST *ast)
{
    AsmData function = compile_ast(compiler, ast->function_call.lhs);
    AsmData return_value = asm_context_data_alloc(&compiler->asm_context, ast->data_type);

    AsmData *function_args = malloc(sizeof(*function_args) * ast->function_call.len);

    if (function_args == NULL) {
        ALLOCATION_ERROR();
    }

    for (size_t i = 0; i < ast->function_call.len; ++i) {
        AsmData argument = compile_ast(compiler, ast->function_call.arguments[i]);
        asm_context_push(&compiler->asm_context, argument);
        asm_context_data_free(&compiler->asm_context, argument);
        function_args[i] = asm_data_stack(compiler->asm_context.stack_frame_size, ast->function_call.arguments[i]->data_type);
    }

    asm_context_call_function(&compiler->asm_context, function, return_value);

    for (size_t i = 0; i < ast->function_call.len; ++i) {
        asm_context_data_free(&compiler->asm_context, function_args[i]);
    }

    free(function_args);

    asm_context_data_free(&compiler->asm_context, function);

    return return_value;
}

static AsmData compile_declaration(Compiler *compiler, AST *ast)
{
    Variable variable      = symbol_table_variable(&compiler->table, ast->declaration.name.len, ast->declaration.name.text);
    VariableID variable_id = symbol_table_variable_id(&compiler->table, ast->declaration.name.len, ast->declaration.name.text);

    asm_context_change_stack(&compiler->asm_context, 8);
    asm_context_add_variable_stack_position(&compiler->asm_context, variable_id, compiler->asm_context.stack_frame_size);

    AsmData asm_variable = asm_data_stack_variable(compiler->asm_context.stack_frame_size, variable.data_type);

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

        case AST_FUNCTION_CALL: {
            return compile_function_call(compiler, ast);
        }

        case AST_DECLARATION: {
            return compile_declaration(compiler, ast);
        }
    }
}

void compile(AST *ast, FILE *file)
{
    Compiler compiler = {
        .table = symbol_table_new(),
        .asm_context = asm_context_new(file)
    };

    size_t arguments_cap = 16;
    DataType **arguments = malloc(sizeof(*arguments) * arguments_cap);

    if (arguments == NULL) {
        ALLOCATION_ERROR();
    }

    arguments[0] = data_type_reference(data_type_type(TYPE_INT8));

    symbol_table_add_variable(
        &compiler.table,
        strlen("print"),
        "print",
        (Variable) {
            .data_type = data_type_function(1, arguments_cap, arguments, data_type_type(TYPE_VOID))
        }
    );

    symbol_table_scan(&compiler.table, ast);
    compiler.table.scope_id = 0;

    AsmData data = compile_ast(&compiler, ast);

    asm_context_mov(&compiler.asm_context, asm_data_register(REGISTER_RAX, data.data_type), data);

    asm_context_data_free(&compiler.asm_context, data);
    asm_context_free(&compiler.asm_context);
    symbol_table_free(&compiler.table);
}
