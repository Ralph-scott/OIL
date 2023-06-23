#define _GNU_SOURCE // needed for asprintf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compile.h"
#include "parser.h"
#include "symbol_table.h"
#include "utils.h"

AsmData compile_ast(Compiler *compiler, const AST *ast);

static void compiler_codegen_data_name(Compiler *compiler, AsmData *data)
{
    switch (data->type) {
        case ASM_REGISTER: {
            fprintf(compiler->file, "%s", REGISTER_TO_STRING[data->asm_register]);
            break;
        }

        case ASM_STACK_REGISTER: {
            fprintf(compiler->file, "QWORD [rsp + %zu]", 8 * (compiler->stack_registers_used - data->stack_register - 1));
            break;
        }

        case ASM_VARIABLE: {
            fprintf(compiler->file, "QWORD [rsp + %zu]", 8 * (compiler->stack_registers_used + compiler->table.variable_id - data->variable.id - 1));
            break;
        }

        case ASM_VOID: {
            ERROR("Tried to access void, but dissolved into nothingness.");
        }
    }
}

static AsmData compiler_alloc_register(Compiler *compiler)
{
    if (compiler->registers_len > 0) {
        return (AsmData) {
            .type = ASM_REGISTER,
            // allocate a new register
            .asm_register = compiler->registers[--compiler->registers_len]
        };
    } else {
        if (compiler->stack_register_pool_len > 0) {
            return (AsmData) {
                .type = ASM_STACK_REGISTER,
                .stack_register = compiler->stack_register_pool[--compiler->stack_register_pool_len]
            };
        }

        fprintf(
            compiler->file,
            "    sub rsp, 8\n"
        );
        return (AsmData) {
            .type = ASM_STACK_REGISTER,
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
        fprintf(compiler->file, "    mov rax, ");
        compiler_codegen_data_name(compiler, output);
        fprintf(compiler->file, "\n");

        fprintf(compiler->file, "    mov ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, ", rax\n");
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
        fprintf(compiler->file, "    mov rax, ");
        compiler_codegen_data_name(compiler, output);
        fprintf(compiler->file, "\n");

        fprintf(compiler->file, "    add ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, ", rax\n");
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
        fprintf(compiler->file, "    mov rax, ");
        compiler_codegen_data_name(compiler, output);
        fprintf(compiler->file, "\n");

        fprintf(compiler->file, "    sub ");
        compiler_codegen_data_name(compiler, input);
        fprintf(compiler->file, ", rax\n");
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
    fprintf(compiler->file, "    mov rax, ");
    compiler_codegen_data_name(compiler, output);
    fprintf(compiler->file, "\n");

    fprintf(compiler->file, "    imul ");
    compiler_codegen_data_name(compiler, input);
    fprintf(compiler->file, "\n");

    fprintf(compiler->file, "    mov ");
    compiler_codegen_data_name(compiler, input);
    fprintf(compiler->file, ", rax\n");
}

static void compiler_div(Compiler *compiler, AsmData *input, AsmData *output)
{
    fprintf(compiler->file, "    mov rax, ");
    compiler_codegen_data_name(compiler, output);
    fprintf(compiler->file, "\n");

    fprintf(compiler->file, "    idiv ");
    compiler_codegen_data_name(compiler, input);
    fprintf(compiler->file, "\n");

    fprintf(compiler->file, "    mov ");
    compiler_codegen_data_name(compiler, input);
    fprintf(compiler->file, ", rax\n");
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
            AsmData asm_register = compiler_alloc_register(compiler);

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

AsmData compile_node(Compiler *compiler, const Token *token)
{
    switch (token->type) {
        case TOKEN_IDENT: {
            Variable variable = symbol_table_variable(&compiler->table, token->text, token->len);

            return (AsmData) {
                .type = ASM_VARIABLE,
                .variable = variable
            };
        }

        case TOKEN_NUMBER: {
            AsmData asm_register = compiler_alloc_register(compiler);

            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &asm_register);
            fprintf(compiler->file, ", %.*s\n", (int) token->len, token->text);

            return asm_register;
        }

        default: {
            UNREACHABLE();
        }
    }
}

static AsmData compile_infix(Compiler *compiler, const ASTInfix *infix)
{
    switch (infix->oper.type) {
        case TOKEN_OPER_ADD: {
            AsmData lhs = compile_ast(compiler, infix->lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->rhs);
            compiler_rvalue(compiler, &rhs);

            compiler_add(compiler, &lhs, &rhs);

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_SUB: {
            AsmData lhs = compile_ast(compiler, infix->lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->rhs);
            compiler_rvalue(compiler, &rhs);

            compiler_sub(compiler, &lhs, &rhs);

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_MUL: {
            AsmData lhs = compile_ast(compiler, infix->lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->rhs);
            compiler_rvalue(compiler, &rhs);

            compiler_mul(compiler, &lhs, &rhs);

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_DIV: {
            AsmData lhs = compile_ast(compiler, infix->lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->rhs);
            compiler_rvalue(compiler, &rhs);

            compiler_div(compiler, &lhs, &rhs);

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_GT: {
            AsmData lhs = compile_ast(compiler, infix->lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->rhs);
            compiler_rvalue(compiler, &rhs);

            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    setg al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", rax\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_LT: {
            AsmData lhs = compile_ast(compiler, infix->lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->rhs);
            compiler_rvalue(compiler, &rhs);

            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    setl al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", rax\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_GT_OR_EQUALS: {
            AsmData lhs = compile_ast(compiler, infix->lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->rhs);
            compiler_rvalue(compiler, &rhs);

            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    setge al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", rax\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_LT_OR_EQUALS: {
            AsmData lhs = compile_ast(compiler, infix->lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->rhs);
            compiler_rvalue(compiler, &rhs);

            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    setle al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", rax\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_EQUALS: {
            AsmData lhs = compile_ast(compiler, infix->lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->rhs);
            compiler_rvalue(compiler, &rhs);

            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    sete al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", rax\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_OPER_NOT_EQUALS: {
            AsmData lhs = compile_ast(compiler, infix->lhs);
            compiler_rvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->rhs);
            compiler_rvalue(compiler, &rhs);


            fprintf(compiler->file, "    cmp ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", ");
            compiler_codegen_data_name(compiler, &rhs);
            fprintf(compiler->file, "\n");
            fprintf(compiler->file, "    setne al\n");
            fprintf(compiler->file, "    mov ");
            compiler_codegen_data_name(compiler, &lhs);
            fprintf(compiler->file, ", rax\n");

            compiler_free_value(compiler, &rhs);

            return lhs;
        }

        case TOKEN_ASSIGN: {
            AsmData lhs = compile_ast(compiler, infix->lhs);
            compiler_lvalue(compiler, &lhs);

            AsmData rhs = compile_ast(compiler, infix->rhs);
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

static AsmData compile_prefix(Compiler *compiler, const ASTPrefix *prefix)
{
    AsmData node = compile_ast(compiler, prefix->node);
    compiler_rvalue(compiler, &node);

    switch (prefix->oper.type) {
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
            fprintf(compiler->file, ", rax\n");
            break;
        }

        default: {
            UNREACHABLE();
        }
    }

    return node;
}

static AsmData compile_block(Compiler *compiler, const ASTBlock *block)
{
    AsmData statement = { .type = ASM_VOID };

    symbol_table_begin_scope(&compiler->table);

    for (size_t i = 0; i < block->len; ++i) {
        statement = compile_ast(compiler, block->statements[i]);
        if (i == block->len - 1) {
            break;
        }
        compiler_free_value(compiler, &statement);
    }

    symbol_table_end_scope(&compiler->table);

    return statement;
}

static AsmData compile_if_statement(Compiler *compiler, const ASTIfStatement *if_statement)
{
    const size_t if_label  = compiler->label_count++;
    const size_t end_label = compiler->label_count++;

    AsmData result = compiler_alloc_register(compiler);
    AsmData condition = compile_ast(compiler, if_statement->condition);
    compiler_rvalue(compiler, &condition);

    fprintf(compiler->file, "    mov rax, ");
    compiler_codegen_data_name(compiler, &condition);
    fprintf(compiler->file, "\n");
    fprintf(compiler->file, "    test rax, rax\n");
    fprintf(compiler->file, "    jz .L%zu\n", if_label);

    compiler_free_value(compiler, &condition);

    AsmData if_block = compile_ast(compiler, if_statement->if_branch);
    compiler_rvalue(compiler, &if_block);

    compiler_mov(compiler, &result, &if_block);

    compiler_free_value(compiler, &if_block);

    if (if_statement->else_branch != NULL) {
        fprintf(compiler->file, "    jmp .L%zu\n", end_label);
    }

    fprintf(compiler->file, ".L%zu:\n", if_label);

    if (if_statement->else_branch != NULL) {
        AsmData else_block = compile_ast(compiler, if_statement->else_branch);
        compiler_rvalue(compiler, &else_block);

        compiler_mov(compiler, &result, &else_block);

        compiler_free_value(compiler, &else_block);
    }

    fprintf(compiler->file, ".L%zu:\n", end_label);

    if (if_statement->else_branch == NULL) {
        return (AsmData) { .type = ASM_VOID };
    } else {
        return result;
    }
}

static AsmData compile_while_loop(Compiler *compiler, const ASTWhileLoop *while_loop)
{
    const size_t start_label = compiler->label_count++;
    const size_t end_label   = compiler->label_count++;

    fprintf(compiler->file, ".L%zu:\n", start_label);

    AsmData condition = compile_ast(compiler, while_loop->condition);

    fprintf(compiler->file, "    mov rax, ");
    compiler_codegen_data_name(compiler, &condition);
    fprintf(compiler->file, "\n");
    fprintf(compiler->file, "    test rax, rax\n");
    fprintf(compiler->file, "    jz .L%zu\n", end_label);

    compiler_free_value(compiler, &condition);

    AsmData body = compile_ast(compiler, while_loop->body);
    compiler_free_value(compiler, &body);

    fprintf(compiler->file, "    jmp .L%zu\n", start_label);
    fprintf(compiler->file, ".L%zu:\n", end_label);

    return (AsmData) { .type = ASM_VOID };
}

static AsmData compile_declaration(Compiler *compiler, const ASTDeclaration *declaration)
{
    Variable variable = symbol_table_variable(&compiler->table, declaration->name.text, declaration->name.len);

    AsmData asm_variable = {
        .type = ASM_VARIABLE,
        .variable = variable
    };

    if (declaration->value != NULL) {
        AsmData value = compile_ast(compiler, declaration->value);
        compiler_rvalue(compiler, &value);

        compiler_mov(compiler, &asm_variable, &value);

        compiler_free_value(compiler, &value);
    }
    
    return (AsmData) { .type = ASM_VOID };
}

AsmData compile_ast(Compiler *compiler, const AST *ast)
{
    switch (ast->type) {
        case AST_NODE: {
            return compile_node(compiler, &ast->node);
        }

        case AST_INFIX: {
            return compile_infix(compiler, &ast->infix);
        }

        case AST_PREFIX: {
            return compile_prefix(compiler, &ast->prefix);
        }

        case AST_BLOCK: {
            return compile_block(compiler, &ast->block);
        }

        case AST_IF_STATEMENT: {
            return compile_if_statement(compiler, &ast->if_statement);
        }

        case AST_WHILE_LOOP: {
            return compile_while_loop(compiler, &ast->while_loop);
        }

        case AST_DECLARATION: {
            return compile_declaration(compiler, &ast->declaration);
        }
    }
}

void compile(const AST *ast, FILE *file)
{
    Compiler compiler = {
        .file = file,

        .stack_registers_used = 0,
        .stack_register_pool_len = 0,
        .stack_register_pool_cap = 512,

        .label_count = 0,
        .registers_len = REGISTER_TYPES,
        .registers = {
            REGISTER_R11,
            REGISTER_R10,
            REGISTER_R9,
            REGISTER_R8,
            REGISTER_RSI,
            REGISTER_RDI,
            REGISTER_RCX
        },
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

    fprintf(compiler.file, "    mov rax, ");
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
