#ifndef AIR_H
#define AIR_H

#include "defs.h"
#include <stdint.h>
#include <stddef.h>

typedef enum {
    OPERAND_REG,
    OPERAND_MEM,
    OPERAND_IMM,
    OPERAND_NONE,
} air_operand_type_t;

typedef struct {
    air_operand_type_t type;
    union {
        struct {
            reg_id_t id;
            reg_size_t size;
        } reg;
        struct {
            reg_id_t base;
            reg_id_t index;
            uint8_t factor;
            int32_t disp;
            addr_size_t size;
        } mem;
        struct {
            int64_t value;
            operand_size_t size;
        } imm;
    };
} air_operand_t;

typedef enum {
    AIR_PUSH,
    AIR_MOV,
    AIR_UNKNOWN,
} air_instr_type_t;

typedef struct air_instr_s {
    air_instr_type_t type;
    air_operand_t operands[2];
    struct air_instr_s *next;
} air_instr_t;

typedef struct {
    air_instr_t *head;
    air_instr_t *tail;
    size_t count;
} air_instr_list_t;

air_instr_list_t *air_instr_list_new();
void air_instr_list_free(air_instr_list_t *);

air_instr_t *air_instr_new();

void air_instr_list_add(air_instr_list_t *, air_instr_t *);

#endif // AIR_H
