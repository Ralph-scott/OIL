#include <stdlib.h>
#include <string.h>
#include "asm_context.h"
#include "types.h"
#include "utils.h"

AsmData asm_data_register(AsmRegister asm_register, DataType *data_type)
{
    return (AsmData) {
        .storage = STORAGE_REGISTER,
        .data_type = data_type,
        .asm_register = asm_register
    };
}

AsmData asm_data_stack(size_t stack_location, DataType *data_type)
{
    return (AsmData) {
        .storage = STORAGE_STACK,
        .data_type = data_type,
        .stack_location = stack_location
    };
}

AsmContext asm_context_new(FILE *file)
{
    AsmContext context = {
        .file = file,

        .data_section_len = 0,
        .data_section_cap = 512,

        .stack_frame_size        = 0,
        .stack_register_pool_len = 0,
        .stack_register_pool_cap = 512,

        .label_count = 0,
        .registers_len = ARRAY_LEN(usable_registers)
    };

    context.data_section = malloc(sizeof(*context.data_section) * context.data_section_cap);

    if (context.data_section == NULL) {
        ALLOCATION_ERROR();
    }

    context.stack_register_pool = malloc(sizeof(*context.stack_register_pool) * context.stack_register_pool_cap);

    if (context.stack_register_pool == NULL) {
        ALLOCATION_ERROR();
    }

    context.registers = malloc(sizeof(usable_registers));

    if (context.registers == NULL) {
        ALLOCATION_ERROR();
    }

    memcpy(context.registers, usable_registers, sizeof(usable_registers));

    fprintf(
        context.file,
        "[BITS 64]\n"
        "global _start\n"
        "section .text\n"
        "_start:\n"
    );

    return context;
}

AsmData asm_context_add_to_data_section(AsmContext *context, char *data, size_t data_len, DataType *data_type)
{
    if (context->data_section_len >= context->data_section_cap) {
        while (context->data_section_len >= context->data_section_cap) {
            context->data_section_cap *= 2;
        }

        context->data_section = realloc(
            context->data_section,
            sizeof(*context->data_section) * context->data_section_cap
        );

        if (context->data_section == NULL) {
            ALLOCATION_ERROR();
        }
    }

    context->data_section[context->data_section_len] = (DataSectionThing) {
        .data_len = data_len,
        .data = data
    };

    return (AsmData) {
        .auto_deref = data_type->type != TYPE_REFERENCE,
        .storage = STORAGE_STATIC,
        .data_type = data_type,
        .static_variable_id = context->data_section_len++
    };
}

size_t asm_context_label_new(AsmContext *context)
{
    return context->label_count++;
}

void asm_context_extend_stack(AsmContext *context, size_t bytes)
{
    context->stack_frame_size += bytes;
    fprintf(context->file, "    sub rsp, %zu\n", bytes);
}

AsmData asm_context_data_alloc(AsmContext *context, DataType *data_type)
{
    if (context->registers_len > 0) {
        return asm_data_register(context->registers[--context->registers_len], data_type);
    }

    if (context->stack_register_pool_len > 0) {
        return asm_data_stack(context->stack_register_pool[--context->stack_register_pool_len], data_type);
    }

    asm_context_extend_stack(context, 8);

    return asm_data_stack(context->stack_frame_size, data_type);
}

void asm_context_data_name(AsmContext *context, AsmData data)
{
    if (data.auto_deref) {
        fprintf(context->file, "%s [", INT_TYPE_ASM[data.data_type->type]);
    }

    switch (data.storage) {
        case STORAGE_NULL: {
            UNREACHABLE();
            break;
        }

        case STORAGE_STATIC: {
            fprintf(context->file, "D%zu", data.static_variable_id);
            break;
        }

        case STORAGE_REGISTER: {
            fprintf(context->file, "%s", REGISTER_TO_STRING[data.asm_register][data.data_type->type]);
            break;
        }

        case STORAGE_STACK: {
            if (data.auto_deref) {
                UNREACHABLE();
            }

            fprintf(context->file, "%s [rsp + %zu]", INT_TYPE_ASM[data.data_type->type], context->stack_frame_size - data.stack_location);
            break;
        }
    }

    if (data.auto_deref) {
        fprintf(context->file, "]");
    }
}

void asm_context_data_free(AsmContext *context, AsmData data)
{
    switch (data.storage) {
        case STORAGE_REGISTER: {
            context->registers[context->registers_len++] = data.asm_register;
            if (context->registers_len > ARRAY_LEN(usable_registers)) {
                UNREACHABLE();
            }
            break;
        }

        case STORAGE_STACK: {
            if (context->stack_register_pool_len >= context->stack_register_pool_cap) {
                while (context->stack_register_pool_len >= context->stack_register_pool_cap) {
                    context->stack_register_pool_cap *= 2;
                }

                context->stack_register_pool = realloc(
                    context->stack_register_pool,
                    sizeof(*context->stack_register_pool) * context->stack_register_pool_cap
                );

                if (context->stack_register_pool == NULL) {
                    ALLOCATION_ERROR();
                }
            }

            context->stack_register_pool[context->stack_register_pool_len++] = data.stack_location;
            break;
        }

        default: {
            break;
        }
    }
}

void asm_context_mov(AsmContext *context, AsmData dst, AsmData src)
{
    if (src.storage != STORAGE_REGISTER && dst.storage != STORAGE_REGISTER) {
        fprintf(context->file, "    mov ");
        asm_context_data_name(context, asm_data_register(REGISTER_RAX, src.data_type));
        fprintf(context->file, ", ");
        asm_context_data_name(context, src);
        fprintf(context->file, "\n");

        fprintf(context->file, "    mov ");
        asm_context_data_name(context, dst);
        fprintf(context->file, ", ");
        asm_context_data_name(context, asm_data_register(REGISTER_RAX, src.data_type));
        fprintf(context->file, "\n");
    } else {
        fprintf(context->file, "    mov ");
        asm_context_data_name(context, dst);
        fprintf(context->file, ", ");
        asm_context_data_name(context, src);
        fprintf(context->file, "\n");
    }
}

void asm_context_mov_constant(AsmContext *context, AsmData dst, size_t constant)
{
    fprintf(context->file, "    mov ");
    asm_context_data_name(context, dst);
    fprintf(context->file, ", %zu\n", constant);
}

void asm_context_add(AsmContext *context, AsmData dst, AsmData src)
{
    if (src.storage != STORAGE_REGISTER) {
        asm_context_mov(context, asm_data_register(REGISTER_RAX, src.data_type), src);

        fprintf(context->file, "    add ");
        asm_context_data_name(context, dst);
        fprintf(context->file, ", ");
        asm_context_data_name(context, asm_data_register(REGISTER_RAX, src.data_type));
        fprintf(context->file, "\n");
    } else {
        fprintf(context->file, "    add ");
        asm_context_data_name(context, dst);
        fprintf(context->file, ", ");
        asm_context_data_name(context, src);
        fprintf(context->file, "\n");
    }
}

void asm_context_sub(AsmContext *context, AsmData dst, AsmData src)
{
    if (src.storage != STORAGE_REGISTER) {
        asm_context_mov(context, asm_data_register(REGISTER_RAX, src.data_type), src);

        fprintf(context->file, "    sub ");
        asm_context_data_name(context, dst);
        fprintf(context->file, ", ");
        asm_context_data_name(context, asm_data_register(REGISTER_RAX, src.data_type));
        fprintf(context->file, "\n");
    } else {
        fprintf(context->file, "    sub ");
        asm_context_data_name(context, dst);
        fprintf(context->file, ", ");
        asm_context_data_name(context, src);
        fprintf(context->file, "\n");
    }
}

void asm_context_mul(AsmContext *context, AsmData dst, AsmData src)
{
    asm_context_mov(context, asm_data_register(REGISTER_RAX, dst.data_type), dst);

    fprintf(context->file, "    mul ");
    asm_context_data_name(context, src);
    fprintf(context->file, "\n");

    asm_context_mov(context, dst, asm_data_register(REGISTER_RAX, dst.data_type));
}

void asm_context_div(AsmContext *context, AsmData dst, AsmData src)
{
    asm_context_mov(context, asm_data_register(REGISTER_RAX, dst.data_type), dst);

    fprintf(context->file, "    div ");
    asm_context_data_name(context, src);
    fprintf(context->file, "\n");

    asm_context_mov(context, dst, asm_data_register(REGISTER_RAX, dst.data_type));
}

void asm_context_reference(AsmContext *context, AsmData dst, AsmData src)
{
    if (dst.storage != STORAGE_REGISTER) {
        fprintf(context->file, "    lea ");
        asm_context_data_name(context, asm_data_register(REGISTER_RAX, src.data_type));
        fprintf(context->file, ", ");
        asm_context_data_name(context, src);
        fprintf(context->file, "\n");

        fprintf(context->file, "    lea ");
        asm_context_data_name(context, dst);
        fprintf(context->file, ", ");
        asm_context_data_name(context, asm_data_register(REGISTER_RAX, src.data_type));
        fprintf(context->file, "\n");
    } else {
        fprintf(context->file, "    lea ");
        asm_context_data_name(context, dst);
        fprintf(context->file, ", ");
        asm_context_data_name(context, src);
        fprintf(context->file, "\n");
    }
}

AsmData asm_context_dereference(AsmContext *context, AsmData dst)
{
    if (dst.storage == STORAGE_REGISTER) {
        dst.auto_deref = true;
        return dst;
    }

    asm_context_mov(context, asm_data_register(REGISTER_RAX, dst.data_type), dst);

    AsmData data = asm_context_data_alloc(context, dst.data_type);

    asm_context_mov(context, data, asm_data_register(REGISTER_RAX, dst.data_type));

    data.auto_deref = true;

    return data;
}

void asm_context_negate(AsmContext *context, AsmData dst)
{
    fprintf(context->file, "    neg ");
    asm_context_data_name(context, dst);
    fprintf(context->file, "\n");
}

void asm_context_cmp(AsmContext *context, AsmData lhs, AsmData rhs)
{
    if (lhs.storage != STORAGE_REGISTER) {
        asm_context_mov(context, asm_data_register(REGISTER_RAX, lhs.data_type), lhs);

        fprintf(context->file, "    cmp ");
        asm_context_data_name(context, asm_data_register(REGISTER_RAX, lhs.data_type));
        fprintf(context->file, ", ");
        asm_context_data_name(context, rhs);
        fprintf(context->file, "\n");
    } else {
        fprintf(context->file, "    cmp ");
        asm_context_data_name(context, lhs);
        fprintf(context->file, ", ");
        asm_context_data_name(context, rhs);
        fprintf(context->file, "\n");
    }
}

void asm_context_test(AsmContext *context, AsmData lhs, AsmData rhs)
{
    if (lhs.storage != STORAGE_REGISTER) {
        asm_context_mov(context, asm_data_register(REGISTER_RAX, lhs.data_type), lhs);

        fprintf(context->file, "    test ");
        asm_context_data_name(context, asm_data_register(REGISTER_RAX, lhs.data_type));
        fprintf(context->file, ", ");
        asm_context_data_name(context, rhs);
        fprintf(context->file, "\n");
    } else {
        fprintf(context->file, "    test ");
        asm_context_data_name(context, lhs);
        fprintf(context->file, ", ");
        asm_context_data_name(context, rhs);
        fprintf(context->file, "\n");
    }
}

void asm_context_setz(AsmContext *context, AsmData dst)
{
    fprintf(context->file, "    setz al\n");
    fprintf(context->file, "    and rax, 0xff\n");
    asm_context_mov(context, dst, asm_data_register(REGISTER_RAX, dst.data_type));
}

void asm_context_setnz(AsmContext *context, AsmData dst)
{
    fprintf(context->file, "    setnz al\n");
    fprintf(context->file, "    and rax, 0xff\n");
    asm_context_mov(context, dst, asm_data_register(REGISTER_RAX, dst.data_type));
}

void asm_context_setl(AsmContext *context, AsmData dst)
{
    fprintf(context->file, "    setl al\n");
    fprintf(context->file, "    and rax, 0xff\n");
    asm_context_mov(context, dst, asm_data_register(REGISTER_RAX, dst.data_type));
}

void asm_context_setg(AsmContext *context, AsmData dst)
{
    fprintf(context->file, "    setg al\n");
    fprintf(context->file, "    and rax, 0xff\n");
    asm_context_mov(context, dst, asm_data_register(REGISTER_RAX, dst.data_type));
}

void asm_context_setle(AsmContext *context, AsmData dst)
{
    fprintf(context->file, "    setle al\n");
    fprintf(context->file, "    and rax, 0xff\n");
    asm_context_mov(context, dst, asm_data_register(REGISTER_RAX, dst.data_type));
}

void asm_context_setge(AsmContext *context, AsmData dst)
{
    fprintf(context->file, "    setge al\n");
    fprintf(context->file, "    and rax, 0xff\n");
    asm_context_mov(context, dst, asm_data_register(REGISTER_RAX, dst.data_type));
}

void asm_context_jmp(AsmContext *context, size_t label_id)
{
    fprintf(context->file, "    jmp .L%zu\n", label_id);
}

void asm_context_jz(AsmContext *context, size_t label_id)
{
    fprintf(context->file, "    jz .L%zu\n", label_id);
}

void asm_context_jnz(AsmContext *context, size_t label_id)
{
    fprintf(context->file, "    jnz .L%zu\n", label_id);
}

void asm_context_label(AsmContext *context, size_t label_id)
{
    fprintf(context->file, ".L%zu:\n", label_id);
}

void asm_context_free(AsmContext *context)
{
    fprintf(
        context->file,
        "    add rsp, %zu\n"
        "exit:\n"
        "    mov rdi, rax\n"
        "    mov rax, 60\n"
        "    syscall\n"
        "section .data\n",
        context->stack_frame_size
    );

    for (size_t i = 0; i < context->data_section_len; ++i) {
        fprintf(context->file, "D%zu: db ", i);
        DataSectionThing thing = context->data_section[i];
        for (size_t j = 0; j < thing.data_len; ++j) {
            fprintf(context->file, "0x%02x", thing.data[j]);
            if (j + 1 < thing.data_len) {
                fprintf(context->file, ", ");
            }
        }
        fprintf(context->file, "\n");
        free(thing.data);
    }

    free(context->data_section);
    free(context->stack_register_pool);
    free(context->registers);
}
