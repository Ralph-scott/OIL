#ifndef COMPILE_H_
#define COMPILE_H_

#include <stdio.h>
#include "hashmap.h"
#include "symbol_table.h"
#include "parser.h"
#include "types.h"

typedef enum AsmDataType
{
    ASM_REGISTER,
    ASM_STACK_REGISTER,
    ASM_VARIABLE,
    ASM_VOID,
} AsmDataType;

typedef enum AsmRegister
{
    REGISTER_RAX,
    REGISTER_RBX,
    REGISTER_RCX,
    REGISTER_RDX,
    REGISTER_RSI,
    REGISTER_RDI,
    REGISTER_R8,
    REGISTER_R9,
    REGISTER_R10,
    REGISTER_R11,
    REGISTER_RSP,
    REGISTER_RBP,

    REGISTER_TYPES
} AsmRegister;

static const char *REGISTER_TO_STRING[REGISTER_TYPES][DATA_TYPES] = {
    [REGISTER_RAX] = {
        [TYPE_INT64] = "rax",
        [TYPE_INT32] = "eax",
        [TYPE_INT16] = "ax",
        [TYPE_INT8]  = "al",
    },
    [REGISTER_RBX] = {
        [TYPE_INT64] = "rbx",
        [TYPE_INT32] = "ebx",
        [TYPE_INT16] = "bx",
        [TYPE_INT8]  = "bl",
    },
    [REGISTER_RCX] = {
        [TYPE_INT64] = "rcx",
        [TYPE_INT32] = "ecx",
        [TYPE_INT16] = "cx",
        [TYPE_INT8]  = "cl",
    },
    [REGISTER_RDX] = {
        [TYPE_INT64] = "rdx",
        [TYPE_INT32] = "edx",
        [TYPE_INT16] = "dx",
        [TYPE_INT8]  = "dl",
    },
    [REGISTER_RSI] = {
        [TYPE_INT64] = "rsi",
        [TYPE_INT32] = "esi",
        [TYPE_INT16] = "si",
        [TYPE_INT8]  = "sil",
    },
    [REGISTER_RDI] = {
        [TYPE_INT64] = "rdi",
        [TYPE_INT32] = "edi",
        [TYPE_INT8]  = "dil",
    },
    [REGISTER_R8] = {
        [TYPE_INT64] = "r8",
        [TYPE_INT32] = "r8d",
        [TYPE_INT16] = "r8w",
        [TYPE_INT8]  = "r8b",
    },
    [REGISTER_R9] = {
        [TYPE_INT64] = "r9",
        [TYPE_INT32] = "r9d",
        [TYPE_INT16] = "r9w",
        [TYPE_INT8]  = "r9b",
    },
    [REGISTER_R10] = {
        [TYPE_INT64] = "r10",
        [TYPE_INT32] = "r10d",
        [TYPE_INT16] = "r10w",
        [TYPE_INT8]  = "r10b",
    },
    [REGISTER_R11] = {
        [TYPE_INT64] = "r11",
        [TYPE_INT32] = "r11d",
        [TYPE_INT16] = "r11w",
        [TYPE_INT8]  = "r11b",
    },
    [REGISTER_RSP] = {
        [TYPE_INT64] = "rsp",
        [TYPE_INT32] = "esp",
        [TYPE_INT16] = "sp",
        [TYPE_INT8]  = "spl",
    },
    [REGISTER_RBP] = {
        [TYPE_INT64] = "rbp",
        [TYPE_INT32] = "ebp",
        [TYPE_INT16] = "bp",
        [TYPE_INT8]  = "bpl",
    }
};

typedef struct AsmData
{
    AsmDataType type;

    DataTypeName int_type;

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
    AsmRegister *registers;

    SymbolTable table;
} Compiler;

void compile(const AST *ast, FILE *file);

#endif // COMPILE_H_
