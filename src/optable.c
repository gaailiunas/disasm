#include "optable.h"
#include "defs.h"

const uint8_t opcode_table[256] = {
    [0x07] = INSTR_POP_SEG,
    [0x0F] = INSTR_POP_SEG, // 2byte
    [0x17] = INSTR_POP_SEG,
    [0x1F] = INSTR_POP_SEG,

    [0x50] = INSTR_PUSH_REG,
    [0x51] = INSTR_PUSH_REG,
    [0x52] = INSTR_PUSH_REG,
    [0x53] = INSTR_PUSH_REG,
    [0x54] = INSTR_PUSH_REG,
    [0x55] = INSTR_PUSH_REG,
    [0x56] = INSTR_PUSH_REG,
    [0x57] = INSTR_PUSH_REG,

    [0x58] = INSTR_POP_REG,
    [0x59] = INSTR_POP_REG,
    [0x5A] = INSTR_POP_REG,
    [0x5B] = INSTR_POP_REG,
    [0x5C] = INSTR_POP_REG,
    [0x5D] = INSTR_POP_REG,
    [0x5E] = INSTR_POP_REG,
    [0x5F] = INSTR_POP_REG,

    [0x89] = INSTR_MOV_RM_R,

    [0x8F] = INSTR_POP_RM,
};
