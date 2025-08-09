#ifndef FRONTEND_H
#define FRONTEND_H

#include "air.h"
#include "defs.h"
#include <stdint.h>

typedef struct {
    const char *r16;
    const char *r32;
    const char *r64;
} reg_name_t;

extern const reg_name_t reg_names[];
extern const char *op_size_suffixes[3];
extern const char *segment_names[];

const char *get_reg_name(uint8_t reg, reg_size_t size);
const char *get_op_size_suffix(operand_size_t size);
const char *get_segment_name(seg_id_t id);

void print_operand(const air_operand_t *op, reg_size_t size_hint);
void print_instr(const air_instr_t *instr);

#endif // FRONTEND_H
