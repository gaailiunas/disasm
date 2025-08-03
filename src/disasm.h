#ifndef DISASM_H
#define DISASM_H

#include "defs.h"
#include "prefix.h"
#include <stddef.h>
#include <stdint.h>

#define SET_FLAG(flags, x) ((flags) |= (x))
#define HAS_FLAG(flags, x) (((flags) & (x)) != 0)

typedef struct {
    const char *r16;
    const char *r32;
    const char *r64;
} reg_name_t;

typedef struct {
    const uint8_t *start;
    const uint8_t *current;
    const uint8_t *end;
    bool has_rex;
    struct rex_prefix rex;
    uint16_t prefixes;
} disasm_ctx_t;

extern const reg_name_t reg_names[];
extern const char *op_size_suffixes[3];
extern const char *segment_names[];

extern const uint8_t instruction_types[256];

static inline bool check_bounds(const disasm_ctx_t *ctx, size_t needed)
{
    return (ctx->current + needed) < ctx->end;
}

addr_size_t get_addr_size(disasm_ctx_t *ctx);
operand_size_t get_operand_size(disasm_ctx_t *ctx, operand_size_t default_size);
reg_size_t get_reg_size(disasm_ctx_t *ctx, reg_size_t default_size);

const char *get_reg_name(uint8_t reg, reg_size_t size);

void disasm_parse_prefixes(disasm_ctx_t *ctx);
void disasm(const uint8_t *instructions, size_t len);

#endif // DISASM_H
