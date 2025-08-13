#ifndef CONTEXT_H
#define CONTEXT_H

#include "prefix.h"
#include <stdint.h>

#define SET_FLAG(flags, x) ((flags) |= (x))
#define HAS_FLAG(flags, x) (((flags) & (x)) != 0)

typedef struct {
    const uint8_t *start;
    const uint8_t *current;
    const uint8_t *end;
    bool has_rex;
    struct rex_prefix rex;
    uint16_t prefixes;
} disasm_ctx_t;

// extend register with REX.R bit (ModR/M.reg field)
static inline void extend_reg_with_rex_r(disasm_ctx_t *ctx, uint8_t *reg)
{
    if (ctx->has_rex && ctx->rex.r) {
        *reg += 8;
    }
}

// extend register with REX.B bit (ModR/M.rm field)
static inline void extend_reg_with_rex_b(disasm_ctx_t *ctx, uint8_t *reg)
{
    if (ctx->has_rex && ctx->rex.b) {
        *reg += 8;
    }
}

// extend register with REX.X bit (SIB.index field)
static inline void extend_reg_with_rex_x(disasm_ctx_t *ctx, uint8_t *reg)
{
    if (ctx->has_rex && ctx->rex.x) {
        *reg += 8;
    }
}

static inline void reset_ctx(disasm_ctx_t *ctx)
{
    ctx->has_rex = false;
    ctx->prefixes = 0;
}

#endif // CONTEXT_H
