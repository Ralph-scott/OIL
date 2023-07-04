#ifndef ASM_CONTEXT_H_
#define ASM_CONTEXT_H_

#include <stdio.h>
#include "types.h"

typedef enum AsmStorageType
{
    STORAGE_NULL,

    STORAGE_REGISTER,
    STORAGE_STACK
} AsmStorageType;

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

static const AsmRegister usable_registers[] = {
    REGISTER_R11,
    REGISTER_R10,
    REGISTER_R9,
    REGISTER_R8,
    REGISTER_RSI,
    REGISTER_RDI,
    REGISTER_RCX
};

static const char *INT_TYPE_ASM[DATA_TYPES] = {
    [TYPE_INT64]     = "QWORD",
    [TYPE_REFERENCE] = "QWORD",
    [TYPE_INT32]     = "DWORD",
    [TYPE_INT16]     = "WORD",
    [TYPE_INT8]      = "BYTE"
};

static const char *REGISTER_TO_STRING[REGISTER_TYPES][DATA_TYPES] = {
    [REGISTER_RAX] = {
        [TYPE_INT64]     = "rax",
        [TYPE_REFERENCE] = "rax",
        [TYPE_INT32]     = "eax",
        [TYPE_INT16]     = "ax",
        [TYPE_INT8]      = "al",
    },
    [REGISTER_RBX] = {
        [TYPE_INT64]     = "rbx",
        [TYPE_REFERENCE] = "rbx",
        [TYPE_INT32]     = "ebx",
        [TYPE_INT16]     = "bx",
        [TYPE_INT8]      = "bl",
    },
    [REGISTER_RCX] = {
        [TYPE_INT64]     = "rcx",
        [TYPE_REFERENCE] = "rcx",
        [TYPE_INT32]     = "ecx",
        [TYPE_INT16]     = "cx",
        [TYPE_INT8]      = "cl",
    },
    [REGISTER_RDX] = {
        [TYPE_INT64]     = "rdx",
        [TYPE_REFERENCE] = "rdx",
        [TYPE_INT32]     = "edx",
        [TYPE_INT16]     = "dx",
        [TYPE_INT8]      = "dl",
    },
    [REGISTER_RSI] = {
        [TYPE_INT64]     = "rsi",
        [TYPE_REFERENCE] = "rsi",
        [TYPE_INT32]     = "esi",
        [TYPE_INT16]     = "si",
        [TYPE_INT8]      = "sil",
    },
    [REGISTER_RDI] = {
        [TYPE_INT64]     = "rdi",
        [TYPE_REFERENCE] = "rdi",
        [TYPE_INT32]     = "edi",
        [TYPE_INT16]     = "di",
        [TYPE_INT8]      = "dil",
    },
    [REGISTER_R8] = {
        [TYPE_INT64]     = "r8",
        [TYPE_REFERENCE] = "r8",
        [TYPE_INT32]     = "r8d",
        [TYPE_INT16]     = "r8w",
        [TYPE_INT8]      = "r8b",
    },
    [REGISTER_R9] = {
        [TYPE_INT64]     = "r9",
        [TYPE_REFERENCE] = "r9",
        [TYPE_INT32]     = "r9d",
        [TYPE_INT16]     = "r9w",
        [TYPE_INT8]      = "r9b",
    },
    [REGISTER_R10] = {
        [TYPE_INT64]     = "r10",
        [TYPE_REFERENCE] = "r10",
        [TYPE_INT32]     = "r10d",
        [TYPE_INT16]     = "r10w",
        [TYPE_INT8]      = "r10b",
    },
    [REGISTER_R11] = {
        [TYPE_INT64]     = "r11",
        [TYPE_REFERENCE] = "r11",
        [TYPE_INT32]     = "r11d",
        [TYPE_INT16]     = "r11w",
        [TYPE_INT8]      = "r11b",
    },
    [REGISTER_RSP] = {
        [TYPE_INT64]     = "rsp",
        [TYPE_REFERENCE] = "rsp",
        [TYPE_INT32]     = "esp",
        [TYPE_INT16]     = "sp",
        [TYPE_INT8]      = "spl",
    },
    [REGISTER_RBP] = {
        [TYPE_INT64]     = "rbp",
        [TYPE_REFERENCE] = "rbp",
        [TYPE_INT32]     = "ebp",
        [TYPE_INT16]     = "bp",
        [TYPE_INT8]      = "bpl",
    }
};

typedef struct AsmData
{
    bool auto_deref;

    AsmStorageType storage;

    DataType *data_type;

    union {
        AsmRegister asm_register;
        size_t stack_location;
    };
} AsmData;

AsmData asm_data_register(AsmRegister asm_register, DataType *data_type);
AsmData asm_data_stack(size_t stack_location, DataType *data_type);

typedef struct AsmContext
{
    FILE *file;

    size_t label_count;

    size_t stack_frame_size;
    size_t stack_register_pool_len;
    size_t stack_register_pool_cap;
    size_t *stack_register_pool;

    size_t registers_len;
    AsmRegister *registers;
} AsmContext;

AsmContext asm_context_new(FILE *file);

size_t asm_context_label_new(AsmContext *context);

void asm_context_extend_stack(AsmContext *context, size_t bytes);

void asm_context_data_name(AsmContext *context, AsmData data);

AsmData asm_context_data_alloc(AsmContext *context, DataType *data_type);
void asm_context_data_free(AsmContext *context, AsmData data);

void asm_context_mov(AsmContext *context, AsmData dst, AsmData src);
void asm_context_mov_constant(AsmContext *context, AsmData dst, size_t constant);
void asm_context_add(AsmContext *context, AsmData dst, AsmData src);
void asm_context_sub(AsmContext *context, AsmData dst, AsmData src);
void asm_context_mul(AsmContext *context, AsmData dst, AsmData src);
void asm_context_div(AsmContext *context, AsmData dst, AsmData src);
void asm_context_negate(AsmContext *context, AsmData dst);

void asm_context_reference(AsmContext *context, AsmData dst, AsmData src);
AsmData asm_context_dereference(AsmContext *context, AsmData dst);

void asm_context_cmp(AsmContext *context, AsmData lhs, AsmData rhs);
void asm_context_test(AsmContext *context, AsmData lhs, AsmData rhs);

void asm_context_setz(AsmContext *context, AsmData dst);
void asm_context_setnz(AsmContext *context, AsmData dst);
void asm_context_setl(AsmContext *context, AsmData dst);
void asm_context_setg(AsmContext *context, AsmData dst);
void asm_context_setle(AsmContext *context, AsmData dst);
void asm_context_setge(AsmContext *context, AsmData dst);

void asm_context_jmp(AsmContext *context, size_t label_id);
void asm_context_jz(AsmContext *context, size_t label_id);
void asm_context_jnz(AsmContext *context, size_t label_id);
void asm_context_label(AsmContext *context, size_t label_id);

void asm_context_free(AsmContext *context);

#endif // ASM_CONTEXT_H_
