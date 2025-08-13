#ifndef OPTABLE_H
#define OPTABLE_H

#include <stdint.h>

typedef enum {
    INSTR_PUSH_REG = 1,
    INSTR_POP_REG,
    INSTR_POP_SEG,
    INSTR_POP_RM,
    INSTR_MOV_RM_R,

    INSTR_NONE = 0xff,
} instr_type_t;

extern const uint8_t opcode_table[256];

#endif // OPTABLE_H
