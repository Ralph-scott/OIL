#define _GNU_SOURCE // needed for asprintf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compile.h"
#include "parser.h"
#include "type_checker.h"
#include "types.h"
#include "utils.h"

AsmData compile_ast(Compiler *compiler, AST *ast);

static void compiler_codegen_data_name(Compiler *compiler, AsmData *data)
{
    switch (data->type) {
        case ASM_REGISTER: {
            fprintf(compiler->file, "%s", REGISTER_TO_STRING[data->asm_register][data->int_type]);
            break;
        }

        case ASM_STACK_REGISTER: {
            fprintf(compiler->file, "%s [rsp + %zu]", INT_TYPE_ASM[data->int_type], 8 * (compiler->stack_registers_used - data->stack_register - 1));
            break;
        }

        case ASM_VARIABLE: {
            fprintf(compiler->file, "%s [rsp + %zu]", INT_TYPE_ASM[data->int_type], 8 * (compiler->stack_registers_used + compiler->table.variable_id - data->variable.id - 1));
            break;
        }

        case ASM_VOID: {
            ERROR("Tried to access void, but dissolved into nothingness.");
        }
    }
}

static AsmData compiler_alloc_register(Compiler *compiler, DataType int_type)
{
    if (compiler->registers_len > 0) {
        return (AsmData) {
            .type = ASM_REGISTER,
            // allocate a new register
            .int_type = int_type,
            .asm_register = compiler->registers[--compiler->registers_len]
        };
    } else {
        if (compiler->stack_register_pool_len > 0) {
            return (AsmData) {
                .type = ASM_STACK_REGISTER,
                .int_type = int_type,
                .stack_register = compiler->stack_register_pool[--compiler->stack_register_pool_len]
            };
        }

        fprintf(compiler->file, "    sub rsp, 8\n");
        return (AsmData) {
            .type = ASM_STACK_REGISTER,
            .int_type = int_type,
            .stack_register = compiler->stack_registers_used++
        };
    }
}

static void compiler_free_value(Compiler *compiler, const AsmData *data)
{
    switch (data->type) {
        case ASM_REGISTER: {
            // Give the register back to the compiler
            compiler->registers[compiler->registers_len] = data->asm_register;
            if (++compiler->registers_len > REGISTER_TYPES) {
                UNREACHABLE();
            }
            break;
        }

        case ASM_STACK_REGISTER: {
            if (compiler->stack_register_pool_len >= compiler->stack_register_pool_cap) {
                while (compiler->stack_register_pool_len >= compiler->stack_register_pool_cap) {
                    compiler->stack_register_pool_cap *= 2;
                }

                compiler->stack_register_pool = realloc(
                    compiler->stack_register_pool,
                    sizeof(size_t) * compiler->stack_register_pool_cap
                );

                if (compiler->stack_register_pool == NULL) {
                    ALLOCATION_ERROR();
                }
            }

            compiler->stack_register_pool[compiler->stack_register_pool_len++] = data->stack_register;
            break;
        }

        default: {
            break;
        }
    }
}

static void compiler_mov(Compiler *compiler, AsmData *input, AsmData *output)
{
    if (input->type == output->type && input->type != ASM_REGISTER) {
        fprintf(compiler->file, "    mov ");
        compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = input->int_type, .asm_register = REGISTER_RAX });
        fprintf(compiler->file, ", ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, "\n");

        fprintf(compiler->file, "    mov ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, ", ");
        compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = input->int_type, .asm_register = REGISTER_RAX });
        fprintf(compiler->file, "\n");
    } else {
        fprintf(compiler->file, "    mov ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, ", ");
        compiler_codegen_data_name(compiler, output);
        fprintf(compiler->file, "\n");
    }
}

static void compiler_add(Compiler *compiler, AsmData *input, AsmData *output)
{
    if (output->type != ASM_REGISTER) {
        fprintf(compiler->file, "    mov ");
        compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .asm_register = REGISTER_RAX });
        fprintf(compiler->file, ", ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, "\n");

        fprintf(compiler->file, "    add ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, ", ");
        compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = input->int_type, .asm_register = REGISTER_RAX });
        fprintf(compiler->file, "\n");
    } else {
        fprintf(compiler->file, "    add ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, ", ");
        compiler_codegen_data_name(compiler, output);
        fprintf(compiler->file, "\n");
    }
}

static void compiler_sub(Compiler *compiler, AsmData *input, AsmData *output)
{
    if (output->type != ASM_REGISTER) {
        fprintf(compiler->file, "    mov ");
        compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = input->int_type, .asm_register = REGISTER_RAX });
        fprintf(compiler->file, ", ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, "\n");

        fprintf(compiler->file, "    sub ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, ", ");
        compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = input->int_type, .asm_register = REGISTER_RAX });
        fprintf(compiler->file, "\n");
    } else {
        fprintf(compiler->file, "    sub ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, ", ");
        compiler_codegen_data_name(compiler, output);
        fprintf(compiler->file, "\n");
    }
}

static void compiler_mul(Compiler *compiler, AsmData *input, AsmData *output)
{
    fprintf(compiler->file, "    mov ");
    compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = input->int_type, .asm_register = REGISTER_RAX });
    fprintf(compiler->file, ", ");
    compiler_codegen_data_name(compiler, output);
    fprintf(compiler->file, "\n");

    fprintf(compiler->file, "    imul ");
    compiler_codegen_data_name(compiler, input);
    fprintf(compiler->file, "\n");

    fprintf(compiler->file, "    mov ");
    compiler_codegen_data_name(compiler, input);
    fprintf(compiler->file, ", ");
    compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = input->int_type, .asm_register = REGISTER_RAX });
    fprintf(compiler->file, "\n");
}

static void compiler_div(Compiler *compiler, AsmData *input, AsmData *output)
{
    fprintf(compiler->file, "    mov ");
    compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = input->int_type, .asm_register = REGISTER_RAX });
    fprintf(compiler->file, ", ");
    fprintf(compiler->file, "\n");
    compiler_codegen_data_name(compiler, output);
    fprintf(compiler->file, "\n");

    fprintf(compiler->file, "    idiv ");
    compiler_codegen_data_name(compiler, input);
    fprintf(compiler->file, "\n");

    fprintf(compiler->file, "    mov ");
    compiler_codegen_data_name(compiler, input);
    fprintf(compiler->file, ", ");
    compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = input->int_type, .asm_register = REGISTER_RAX });
    fprintf(compiler->file, "\n");
}

static void compiler_lvalue(Compiler *compiler, AsmData *data)
{
    (void) compiler;
    switch (data->type) {
        case ASM_REGISTER:
        case ASM_STACK_REGISTER: {
            ERROR("Go learn how to code.");
            break;
        }

        case ASM_VARIABLE: {
            break;
        }

        case ASM_VOID: {
            ERROR("You can't turn nothing into anything.");
            break;
        }
    }
}

static void compiler_rvalue(Compiler *compiler, AsmData *data)
{
    switch (data->type) {
        case ASM_REGISTER:
        case ASM_STACK_REGISTER: {
            break;
        }

        case ASM_VARIABLE: {
            AsmData asm_register = compiler_alloc_register(compiler, data->int_type);

            compiler_mov(compiler, &asm_register, data);

            compiler_free_value(compiler, data);
            *data = asm_register;
            break;
        }

        default: {
            ERROR("Void is not a thing. Void is nothing.");
        }
    }
}

static AsmData compile_node(Compiler *compiler, AST *node)
{
    switch (node->node.type) {
        case TOKEN_IDENT: {
            Variable variable = symbol_table_variable(&compiler->table, node->node.text, node->node.len);

            return (AsmData) {
                .type = ASM_VARIABLE,
                .int_type = node->data_type,
                .variable = variable
            };
        }

        case TOKEN_NUMBER: {
            AsmData asm_register = compiler_alloc_register(compiler, node->data_type == TYPE_NULL ? TYPE_INT32 : node->data_type);

            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &asm_register);
            fprintf(compiler->file, ", %.*s\n", (int) node->node.len, node->node.text);

            return asm_register;
        }

        default: {
            UNREACHABLE();
        }
    }
}

static AsmData compile_infix(Compiler *compiler, AST *infix)
{
    switch (infix->infix.oper.type) {
        case TOKEN_OPER_ADD: {
            AsmData lhs = compile_ast(compiler, infix->infix.lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->infix.rhs);
            compiler_rvalue(compiler, &rhs);

            compiler_add(compiler, &lhs, &rhs);

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_SUB: {
            AsmData lhs = compile_ast(compiler, infix->infix.lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->infix.rhs);
            compiler_rvalue(compiler, &rhs);

            compiler_sub(compiler, &lhs, &rhs);

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_MUL: {
            AsmData lhs = compile_ast(compiler, infix->infix.lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->infix.rhs);
            compiler_rvalue(compiler, &rhs);

            compiler_mul(compiler, &lhs, &rhs);

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_DIV: {
            AsmData lhs = compile_ast(compiler, infix->infix.lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->infix.rhs);
            compiler_rvalue(compiler, &rhs);

            compiler_div(compiler, &lhs, &rhs);

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_GT: {
            AsmData lhs = compile_ast(compiler, infix->infix.lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->infix.rhs);
            compiler_rvalue(compiler, &rhs);

            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    setg al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = lhs.int_type, .asm_register = REGISTER_RAX });
            fprintf(compiler->file, "\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_LT: {
            AsmData lhs = compile_ast(compiler, infix->infix.lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->infix.rhs);
            compiler_rvalue(compiler, &rhs);

            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    setl al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = lhs.int_type, .asm_register = REGISTER_RAX });
            fprintf(compiler->file, "\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_GT_OR_EQUALS: {
            AsmData lhs = compile_ast(compiler, infix->infix.lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->infix.rhs);
            compiler_rvalue(compiler, &rhs);

            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    setge al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = lhs.int_type, .asm_register = REGISTER_RAX });
            fprintf(compiler->file, "\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_LT_OR_EQUALS: {
            AsmData lhs = compile_ast(compiler, infix->infix.lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->infix.rhs);
            compiler_rvalue(compiler, &rhs);

            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    setle al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = lhs.int_type, .asm_register = REGISTER_RAX });
            fprintf(compiler->file, "\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_EQUALS: {
            AsmData lhs = compile_ast(compiler, infix->infix.lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->infix.rhs);
            compiler_rvalue(compiler, &rhs);

            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    sete al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = lhs.int_type, .asm_register = REGISTER_RAX });
            fprintf(compiler->file, "\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_NOT_EQUALS: {
            AsmData lhs = compile_ast(compiler, infix->infix.lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->infix.rhs);
            compiler_rvalue(compiler, &rhs);

            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    setne al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = lhs.int_type, .asm_register = REGISTER_RAX });
            fprintf(compiler->file, "\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_ASSIGN: {
            AsmData lhs = compile_ast(compiler, infix->infix.lhs);
            compiler_lvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->infix.rhs);
            compiler_rvalue(compiler, &rhs);

            compiler_mov(compiler, &lhs, &rhs);

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        default: {
            UNREACHABLE();
        }
    }
}

static AsmData compile_prefix(Compiler *compiler, AST *ast)
{
    AsmData node = compile_ast(compiler, ast->prefix.node);
    compiler_rvalue(compiler, &node);

    switch (ast->prefix.oper.type) {
        case TOKEN_OPER_SUB: {
            fprintf(compiler->file, "    neg ");
            compiler_codegen_data_name(compiler, &node);
            fprintf(compiler->file, "\n");
            break;
        }

        case TOKEN_NOT: {
            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &node);
            fprintf(compiler->file, ", 0\n");
            fprintf(compiler->file, "    sete al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &node);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = node.int_type, .asm_register = REGISTER_RAX });
            fprintf(compiler->file, "\n");
            break;
        }

        default: {
            UNREACHABLE();
        }
    }

    return node;
}

static AsmData compile_block(Compiler *compiler, AST *ast)
{
    AsmData statement = { .type = ASM_VOID };

    symbol_table_begin_scope(&compiler->table);

    for (size_t i = 0; i < ast->block.len; ++i) {
        statement = compile_ast(compiler, ast->block.statements[i]);
        if (i == ast->block.len - 1) {
            break;
        }
        compiler_free_value(compiler, &statement);
    }

    symbol_table_end_scope(&compiler->table);

    return statement;
}

static AsmData compile_if_statement(Compiler *compiler, AST *ast)
{
    const size_t if_label  = compiler->label_count++;
    const size_t end_label = compiler->label_count++;

    AsmData condition = compile_ast(compiler, ast->if_statement.condition);
    compiler_rvalue(compiler, &condition);

    AsmData result = compiler_alloc_register(compiler, condition.int_type);

    fprintf(compiler->file, "    mov ");
    compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = condition.int_type, .asm_register = REGISTER_RAX });
    fprintf(compiler->file, ", ");
    compiler_codegen_data_name(compiler, &condition);
    fprintf(compiler->file, "\n");
    fprintf(compiler->file, "    test ");
    compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = condition.int_type, .asm_register = REGISTER_RAX });
    fprintf(compiler->file, ", ");
    compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = condition.int_type, .asm_register = REGISTER_RAX });
    fprintf(compiler->file, "\n");
    fprintf(compiler->file, "    jz .L%zu\n", if_label);

    compiler_free_value(compiler, &condition);

    AsmData if_block = compile_ast(compiler, ast->if_statement.if_branch);
    compiler_rvalue(compiler, &if_block);

    compiler_mov(compiler, &result, &if_block);

    compiler_free_value(compiler, &if_block);

    if (ast->if_statement.else_branch != NULL) {
        fprintf(compiler->file, "    jmp .L%zu\n", end_label);
    }

    fprintf(compiler->file, ".L%zu:\n", if_label);

    if (ast->if_statement.else_branch != NULL) {
        AsmData else_block = compile_ast(compiler, ast->if_statement.else_branch);
        compiler_rvalue(compiler, &else_block);

        compiler_mov(compiler, &result, &else_block);

        compiler_free_value(compiler, &else_block);
    }

    fprintf(compiler->file, ".L%zu:\n", end_label);

    if (ast->if_statement.else_branch == NULL) {
        return (AsmData) { .type = ASM_VOID };
    } else {
        return result;
    }
}

static AsmData compile_while_loop(Compiler *compiler, AST *ast)
{
    const size_t start_label = compiler->label_count++;
    const size_t end_label   = compiler->label_count++;

    fprintf(compiler->file, ".L%zu:\n", start_label);

    AsmData condition = compile_ast(compiler, ast->while_loop.condition);

    fprintf(compiler->file, "    mov ");
    compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = condition.int_type, .asm_register = REGISTER_RAX });
    fprintf(compiler->file, ", ");
    compiler_codegen_data_name(compiler, &condition);
    fprintf(compiler->file, "\n");
    fprintf(compiler->file, "    test ");
    compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = condition.int_type, .asm_register = REGISTER_RAX });
    fprintf(compiler->file, ", ");
    compiler_codegen_data_name(compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = condition.int_type, .asm_register = REGISTER_RAX });
    fprintf(compiler->file, "\n");
    fprintf(compiler->file, "    jz .L%zu\n", end_label);

    compiler_free_value(compiler, &condition);

    AsmData body = compile_ast(compiler, ast->while_loop.body);
    compiler_free_value(compiler, &body);

    fprintf(compiler->file, "    jmp .L%zu\n", start_label);
    fprintf(compiler->file, ".L%zu:\n", end_label);

    return (AsmData) { .type = ASM_VOID };
}

static AsmData compile_declaration(Compiler *compiler, AST *ast)
{
    Variable variable = symbol_table_variable(&compiler->table, ast->declaration.name.text, ast->declaration.name.len);

    AsmData asm_variable = {
        .type = ASM_VARIABLE,
        .int_type = variable.data_type,
        .variable = variable
    };

    if (ast->declaration.value != NULL) {
        AsmData value = compile_ast(compiler, ast->declaration.value);
        compiler_rvalue(compiler, &value);

        compiler_mov(compiler, &asm_variable, &value);

        compiler_free_value(compiler, &value);
    }
    
    return (AsmData) { .type = ASM_VOID };
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
    const AsmRegister registers[] = {
        REGISTER_R11,
        REGISTER_R10,
        REGISTER_R9,
        REGISTER_R8,
        REGISTER_RSI,
        REGISTER_RDI,
        REGISTER_RCX
    };

    Compiler compiler = {
        .file = file,

        .stack_registers_used = 0,
        .stack_register_pool_len = 0,
        .stack_register_pool_cap = 512,

        .label_count = 0,
        .registers = (AsmRegister *) registers,
        .registers_len = ARRAY_LEN(registers),
        .table = symbol_table_new(ast)
    };

    compiler.stack_register_pool = malloc(sizeof(size_t) * compiler.stack_register_pool_cap);

    if (compiler.stack_register_pool == NULL) {
        ALLOCATION_ERROR();
    }

    fprintf(
        compiler.file,
        "[BITS 64]\n"
        "global _start\n"
        "section .text\n"
        "_start:\n"
        "    sub rsp, %zu\n",
        8 * compiler.table.variable_id
    );

    AsmData data = compile_ast(&compiler, ast);

    fprintf(compiler.file, "    mov ");
    compiler_codegen_data_name(&compiler, &(AsmData) { .type = ASM_REGISTER, .int_type = data.int_type, .asm_register = REGISTER_RAX });
    fprintf(compiler.file, ", ");
    compiler_codegen_data_name(&compiler, &data);
    fprintf(compiler.file, "\n");
    fprintf(
        compiler.file,
        "    add rsp, %zu\n"
        "exit:\n"
        "    mov rdi, rax\n"
        "    mov rax, 60\n"
        "    syscall\n"
        "section .bss\n",
        8 * (compiler.stack_registers_used + compiler.table.variable_id)
    );

    compiler_free_value(&compiler, &data);
    symbol_table_free(&compiler.table);
}
