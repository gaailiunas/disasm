#ifndef AIR_H
#define AIR_H

#include "defs.h"
#include "sib.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    OPERAND_REG,
    OPERAND_MEM,
    OPERAND_IMM,

    OPERAND_NONE = 0xff,
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
            scale_factor_t factor;
            int32_t disp;
            addr_size_t size;
            operand_size_t op_size; // how many bytes does the data occupy
            seg_id_t segment;
        } mem;
        struct {
            int64_t value;
            operand_size_t size;
        } imm;
    };
} air_operand_t;

typedef enum {
    AIR_POP,
    AIR_PUSH,
    AIR_MOV,

    AIR_UNKNOWN = 0xff,
} air_instr_type_t;

typedef struct air_instr_s {
    air_instr_type_t type;
    union {
        struct {
            air_operand_t dst;
            air_operand_t src;
        } binary;
        struct {
            air_operand_t operand;
        } unary;
    } ops;
    size_t length;
    struct air_instr_s *next;
} air_instr_t;

#define AIR_CHUNK_CAPACITY 128

typedef struct air_instr_chunk_s {
    air_instr_t items[AIR_CHUNK_CAPACITY];
    struct air_instr_chunk_s *next;
} air_instr_chunk_t;

typedef struct {
    air_instr_chunk_t *head;
    air_instr_chunk_t *tail;
    size_t count;
    size_t used_in_tail;
} air_instr_list_t;

void air_instr_list_init(air_instr_list_t *);
air_instr_list_t *air_instr_list_new();

void air_instr_list_destroy(air_instr_list_t *);
void air_instr_list_free(air_instr_list_t *);

air_instr_t *air_instr_list_get_new(air_instr_list_t *);

#endif // AIR_H
