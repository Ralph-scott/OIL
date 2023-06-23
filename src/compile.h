#ifndef COMPILE_H_
#define COMPILE_H_

#include <stdio.h>
#include "hashmap.h"
#include "symbol_table.h"
#include "parser.h"

typedef enum AsmDataType
{
    ASM_REGISTER,
    ASM_STACK_REGISTER,
    ASM_VARIABLE,
    ASM_VOID,
} AsmDataType;

typedef enum AsmRegister
{
    REGISTER_RCX,
    REGISTER_RSI,
    REGISTER_RDI,
    REGISTER_R8,
    REGISTER_R9,
    REGISTER_R10,
    REGISTER_R11,

    REGISTER_TYPES
} AsmRegister;

static const char *REGISTER_TO_STRING[REGISTER_TYPES] = {
    [REGISTER_RCX] = "rcx",
    [REGISTER_RSI] = "rsi",
    [REGISTER_RDI] = "rdi",
    [REGISTER_R8]  = "r8",
    [REGISTER_R9]  = "r9",
    [REGISTER_R10] = "r10",
    [REGISTER_R11] = "r11"
};

typedef struct AsmData
{
    AsmDataType type;

    union {
        AsmRegister asm_register;
        size_t stack_register;
        Variable variable;
    };
} AsmData;

typedef struct Compiler
{
    FILE *file;

    size_t label_count;

    size_t stack_registers_used;
    size_t stack_register_pool_len;
    size_t stack_register_pool_cap;
    size_t *stack_register_pool;

    size_t registers_len;
    AsmRegister registers[REGISTER_TYPES];

    SymbolTable table;
} Compiler;

void compile(const AST *ast, FILE *file);

#endif // COMPILE_H_
