#ifndef DISASM_H
#define DISASM_H

#include <stdint.h>
#include <stddef.h>
#include "prefix.h"

typedef enum {
    /* group 1 */
    INSTR_PREFIX_LOCK = 1 << 0,
    INSTR_PREFIX_REPNE = 1 << 1,
    INSTR_PREFIX_REP_REPE = 1 << 2,
    /* group 2 */
    INSTR_PREFIX_0x2e = 1 << 3,
    INSTR_PREFIX_SS = 1 << 4,
    INSTR_PREFIX_0x3e = 1 << 5,
    INSTR_PREFIX_ES = 1 << 6,
    INSTR_PREFIX_FS = 1 << 7,
    INSTR_PREFIX_GS = 1 << 8,
    INSTR_PREFIX_BRANCH_NOT_TAKEN = 1 << 9,
    INSTR_PREFIX_BRANCH_TAKEN = 1 << 10,
    /* group 3 */
    INSTR_PREFIX_OP = 1 << 11,
    /* group 4 */
    INSTR_PREFIX_ADDR_SIZE = 1 << 12,
} instr_prefix_flag_t;

#define SET_FLAG(flags, x) ((flags) |= (x))
#define HAS_FLAG(flags, x) (((flags) & (x)) != 0)

typedef enum {
    REG_SIZE_16 = 0,
    REG_SIZE_32 = 1,
    REG_SIZE_64 = 2,
} reg_size_t;

typedef enum {
    ADDR_SIZE_32 = 1,
    ADDR_SIZE_64 = 2,
} addr_size_t;

typedef enum {
    OPERAND_SIZE_16 = 0,
    OPERAND_SIZE_32 = 1,
    OPERAND_SIZE_64 = 2,
} operand_size_t;

typedef struct {
    const char *r16;
    const char *r32;
    const char *r64;
} reg_name_t;

typedef enum {
    INSTR_PUSH_REG = 1,
    INSTR_MOV_RM_R,
    INSTR_UNKNOWN,
} instr_type_t;

typedef struct {
    const uint8_t *start;
    const uint8_t *current;
    const uint8_t *end;
    bool has_rex;
    struct rex_prefix rex;
    uint16_t prefixes;
} disasm_ctx_t;

enum disasm_register_64 {
    REG_RAX,
    REG_RCX,
    REG_RDX,
    REG_RBX,
    REG_RSP,
    REG_RBP,
    REG_RSI,
    REG_RDI,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
};

enum disasm_register_32 {
    REG_EAX,
    REG_ECX,
    REG_EDX,
    REG_EBX,
    REG_ESP,
    REG_EBP,
    REG_ESI,
    REG_EDI,
};

enum disasm_register_16 {
    REG_AX,
    REG_CX,
    REG_DX,
    REG_BX,
    REG_SP,
    REG_BP,
    REG_SI,
    REG_DI,
};

extern const reg_name_t reg_names[];
extern const char *op_size_suffixes[3];

extern const uint8_t instruction_types[256];

static inline bool check_bounds(const disasm_ctx_t *ctx, size_t needed)
{
    return (ctx->current + needed) < ctx->end;
}

addr_size_t get_addr_size(disasm_ctx_t *ctx);
operand_size_t get_operand_size(disasm_ctx_t *ctx);

const char *get_reg_name(uint8_t reg, reg_size_t size);

void disasm_parse_prefixes(disasm_ctx_t *ctx);
void disasm(const uint8_t *instructions, size_t len);

#endif // DISASM_H
