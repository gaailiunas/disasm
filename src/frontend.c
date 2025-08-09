#include "frontend.h"
#include <stdbool.h>
#include <stdio.h>

const reg_name_t reg_names[] = {
    {"ax", "eax", "rax"},
    {"cx", "ecx", "rcx"},
    {"dx", "edx", "rdx"},
    {"bx", "ebx", "rbx"},
    {"sp", "esp", "rsp"},
    {"bp", "ebp", "rbp"},
    {"si", "esi", "rsi"},
    {"di", "edi", "rdi"},
    {"r8w", "r8d", "r8"},
    {"r9w", "r9d", "r9"},
    {"r10w", "r10d", "r10"},
    {"r11w", "r11d", "r11"},
    {"r12w", "r12d", "r12"},
    {"r13w", "r13d", "r13"},
    {"r14w", "r14d", "r14"},
    {"r15w", "r15d", "r15"},
    {"ip", "eip", "rip"},
};

const char *op_size_suffixes[3] = {
    "word",
    "dword",
    "qword",
};

const char *segment_names[] = {
    "es",
    "cs",
    "ss",
    "ds",
    "fs",
    "gs",
};

const char *get_reg_name(uint8_t reg, reg_size_t size)
{
    if (reg > REG_IP) {
        printf("going outside bounds. reg: %u\n", reg);
        return "unk";
    }

    switch (size) {
    case REG_SIZE_16:
        return reg_names[reg].r16;
    case REG_SIZE_32:
        return reg_names[reg].r32;
    case REG_SIZE_64:
        return reg_names[reg].r64;
    default:
        return reg_names[reg].r32;
    }
}

const char *get_op_size_suffix(operand_size_t size)
{
    if (size > OPERAND_SIZE_64)
        return "unk_size";
    return op_size_suffixes[size];
}

const char *get_segment_name(seg_id_t id)
{
    if (id > SEG_GS)
        return "unk";
    return segment_names[id];
}

void print_operand(const air_operand_t *op, reg_size_t size_hint)
{
    switch (op->type) {
    case OPERAND_REG: {
        const char *reg_name = get_reg_name(op->reg.id, op->reg.size);
        printf("%s", reg_name);
        break;
    }
    case OPERAND_MEM: {
        if (op->mem.segment != SEG_NONE) {
            printf("%s", get_segment_name(op->mem.segment));
            break;
        }

        const char *size_str = get_op_size_suffix((operand_size_t)size_hint);
        printf("%s ptr [", size_str);

        bool need_plus = false;

        if (op->mem.base != REG_NONE) {
            printf("%s", get_reg_name(op->mem.base, (reg_size_t)op->mem.size));
            need_plus = true;
        }

        if (op->mem.index != REG_NONE) {
            if (need_plus)
                printf("+");
            printf("%s*%d",
                get_reg_name(op->mem.index, (reg_size_t)op->mem.size),
                op->mem.factor);
            need_plus = true;
        }

        int32_t disp = op->mem.disp;

        if (disp != 0 || (!need_plus && op->mem.base == REG_NONE &&
                             op->mem.index == REG_NONE)) {
            if (disp < 0) {
                printf("-%#x", -disp);
            }
            else {
                if (need_plus)
                    printf("+");
                printf("%#x", disp);
            }
        }

        printf("]");
        break;
    }
    case OPERAND_IMM: {
        printf("0x%llx", (unsigned long long)op->imm.value);
        break;
    }
    default:
        printf("<?>");
        break;
    }
}

void print_instr(const air_instr_t *instr)
{
    switch (instr->type) {
    case AIR_POP: {
        const air_operand_t *op = &instr->ops.unary.operand;
        printf("pop ");
        if (op->type == OPERAND_REG) {
            print_operand(op, op->reg.size);
        }
        else if (op->type == OPERAND_MEM) {
            print_operand(op, (reg_size_t)op->mem.op_size);
        }
        printf("\n");
        break;
    }
    case AIR_PUSH: {
        const air_operand_t *op = &instr->ops.binary.dst;
        printf("push ");
        if (op->type == OPERAND_REG) {
            print_operand(op, op->reg.size);
        }
        printf("\n");
        break;
    }
    case AIR_MOV: {
        const air_operand_t *dst = &instr->ops.binary.dst;
        const air_operand_t *src = &instr->ops.binary.src;

        printf("mov ");

        if (dst->type == OPERAND_MEM && src->type == OPERAND_REG) {
            print_operand(dst, src->reg.size);
            printf(", ");
            print_operand(src, src->reg.size);
        }
        else if (dst->type == OPERAND_REG && src->type == OPERAND_MEM) {
            print_operand(dst, dst->reg.size);
            printf(", ");
            print_operand(src, dst->reg.size);
        }
        else { // fallback
            print_operand(dst, REG_SIZE_64);
            printf(", ");
            print_operand(src, REG_SIZE_64);
        }

        printf("\n");
        break;
    }
    default:
        printf("unknown or unimplemented instruction (type %d)\n", instr->type);
        break;
    }
}
