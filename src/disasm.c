#include "disasm.h"
#include "air.h"
#include "defs.h"
#include "modrm.h"
#include "prefix.h"
#include "sib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const uint8_t instruction_types[256] = {
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

addr_size_t get_addr_size(disasm_ctx_t *ctx)
{
    return HAS_FLAG(ctx->prefixes, INSTR_PREFIX_ADDR_SIZE) ? ADDR_SIZE_32
                                                           : ADDR_SIZE_64;
}

operand_size_t get_operand_size(disasm_ctx_t *ctx, operand_size_t default_size)
{
    if (ctx->has_rex && ctx->rex.w)
        return OPERAND_SIZE_64;
    if (HAS_FLAG(ctx->prefixes, INSTR_PREFIX_OP))
        return OPERAND_SIZE_16;
    return default_size == OPERAND_SIZE_NONE ? OPERAND_SIZE_32 : default_size;
}

reg_size_t get_reg_size(disasm_ctx_t *ctx, reg_size_t default_size)
{
    return (reg_size_t)get_operand_size(ctx, (operand_size_t)default_size);
}

// extend register with REX.R bit (ModR/M.reg field)
static inline void extend_reg_with_rex_r(disasm_ctx_t *ctx, uint8_t *reg)
{
    if (ctx->has_rex && ctx->rex.r)
        *reg += 8;
}

// extend register with REX.B bit (ModR/M.rm field)
static inline void extend_reg_with_rex_b(disasm_ctx_t *ctx, uint8_t *reg)
{
    if (ctx->has_rex && ctx->rex.b)
        *reg += 8;
}

// extend register with REX.X bit (SIB.index field)
static inline void extend_reg_with_rex_x(disasm_ctx_t *ctx, uint8_t *reg)
{
    if (ctx->has_rex && ctx->rex.x)
        *reg += 8;
}

void disasm_parse_prefixes(disasm_ctx_t *ctx)
{
    while (ctx->current < ctx->end) {
        uint8_t byte = *ctx->current;

        switch (byte) {
        case 0x40 ... 0x4f: { // REX
            if (rex_extract(byte, &ctx->rex)) {
                ctx->has_rex = true;
                ctx->current++;
                continue;
            }
            return;
        }
        case PREFIX_REPNE: {
            SET_FLAG(ctx->prefixes, INSTR_PREFIX_REPNE);
            ctx->current++;
            continue;
        }
        case PREFIX_REP_REPE: {
            SET_FLAG(ctx->prefixes, INSTR_PREFIX_REP_REPE);
            ctx->current++;
            continue;
        }
        case PREFIX_LOCK: {
            SET_FLAG(ctx->prefixes, INSTR_PREFIX_LOCK);
            ctx->current++;
            continue;
        }
        case PREFIX_0x2e: {
            SET_FLAG(ctx->prefixes, INSTR_PREFIX_0x2e);
            ctx->current++;
            continue;
        }
        case PREFIX_SEG_SS: {
            SET_FLAG(ctx->prefixes, INSTR_PREFIX_SS);
            ctx->current++;
            continue;
        }
        case PREFIX_0x3e: {
            SET_FLAG(ctx->prefixes, INSTR_PREFIX_0x3e);
            ctx->current++;
            continue;
        }
        case PREFIX_SEG_ES: {
            SET_FLAG(ctx->prefixes, INSTR_PREFIX_ES);
            ctx->current++;
            continue;
        }
        case PREFIX_SEG_FS: {
            SET_FLAG(ctx->prefixes, INSTR_PREFIX_FS);
            ctx->current++;
            continue;
        }
        case PREFIX_SEG_GS: {
            SET_FLAG(ctx->prefixes, INSTR_PREFIX_GS);
            ctx->current++;
            continue;
        }
        case PREFIX_OP_SIZE_OVERRIDE: {
            SET_FLAG(ctx->prefixes, INSTR_PREFIX_OP);
            ctx->current++;
            continue;
        }
        case PREFIX_ADDR_SIZE_OVERRIDE: {
            SET_FLAG(ctx->prefixes, INSTR_PREFIX_ADDR_SIZE);
            ctx->current++;
            continue;
        }
        default:
            return;
        }
    }
}

static inline void reset_ctx(disasm_ctx_t *ctx)
{
    ctx->has_rex = false;
    ctx->prefixes = 0;
}

static inline void init_reg_operand(
    air_operand_t *op, reg_id_t reg, reg_size_t size)
{
    op->type = OPERAND_REG;
    op->reg.id = reg;
    op->reg.size = size;
}

static inline void init_mem_operand(air_operand_t *op, reg_id_t base,
    reg_id_t index, scale_factor_t factor, int32_t disp, addr_size_t size,
    seg_id_t seg, operand_size_t op_size)
{
    op->type = OPERAND_MEM;
    op->mem.base = base;
    op->mem.index = index;
    op->mem.factor = factor;
    op->mem.disp = disp;
    op->mem.size = size;
    op->mem.op_size = op_size;
    op->mem.segment = seg;
}

static int8_t get_disp8(disasm_ctx_t *ctx)
{
    int8_t disp;
    memcpy(&disp, ctx->current, 1);
    ctx->current += 1;
    return disp;
}

static int32_t get_disp32(disasm_ctx_t *ctx)
{
    int32_t disp;
    memcpy(&disp, ctx->current, 4);
    ctx->current += 4;
    return disp;
}

static bool handle_sib_operand(disasm_ctx_t *ctx, struct modrm *mod,
    air_operand_t *mem_op, addr_size_t addr_size, operand_size_t op_size)
{
    if (!check_bounds(ctx, 1)) {
        printf("no sib byte\n");
        return false;
    }

    struct sib s;
    sib_extract(*ctx->current++, &s);
    extend_reg_with_rex_x(ctx, &s.index);
    extend_reg_with_rex_b(ctx, &s.base);

    reg_id_t base_reg = s.base;
    reg_id_t index_reg = (s.index == REG_SP) ? REG_NONE : s.index;
    int32_t disp = 0;

    if (mod->mod == 0) {
        if (s.base == REG_BP || s.base == REG_R13) {
            if (!check_bounds(ctx, 4)) {
                printf("not enough bytes for 4byte disp\n");
                return false;
            }

            disp = get_disp32(ctx);
            base_reg = REG_NONE;
        }
    }
    else if (mod->mod == 1) {
        if (!check_bounds(ctx, 1)) {
            printf("not enough bytes for 1byte disp\n");
            return false;
        }
        disp = get_disp8(ctx);
    }
    else if (mod->mod == 2) {
        if (!check_bounds(ctx, 4)) {
            printf("not enough bytes for 4byte disp\n");
            return false;
        }
        disp = get_disp32(ctx);
    }

    init_mem_operand(mem_op, base_reg, index_reg, s.factor, disp, addr_size,
        SEG_NONE, op_size);
    return true;
}

static bool handle_memory_operand(disasm_ctx_t *ctx, struct modrm *mod,
    air_operand_t *mem_op, operand_size_t op_size)
{
    addr_size_t addr_size = get_addr_size(ctx);

    if (mod->mod == 0) {
        switch (mod->rm) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 6:
        case 7: {
            extend_reg_with_rex_r(ctx, &mod->reg);
            extend_reg_with_rex_b(ctx, &mod->rm);
            init_mem_operand(mem_op, mod->rm, REG_NONE, FACTOR_1, 0, addr_size,
                SEG_NONE, op_size);
            return true;
        }
        case 4: {
            return handle_sib_operand(ctx, mod, mem_op, addr_size, op_size);
        }
        case 5: {
            if (!check_bounds(ctx, 4)) {
                printf("not enough bytes for 4byte disp\n");
                return false;
            }

            extend_reg_with_rex_r(ctx, &mod->reg);
            extend_reg_with_rex_b(ctx, &mod->rm);

            int32_t disp = get_disp32(ctx);
            init_mem_operand(mem_op, REG_IP, REG_NONE, FACTOR_1, disp,
                addr_size, SEG_NONE, op_size);
            return true;
        }
        }
    }

    if (mod->rm == REG_SP) {
        return handle_sib_operand(ctx, mod, mem_op, addr_size, op_size);
    }

    extend_reg_with_rex_r(ctx, &mod->reg);
    extend_reg_with_rex_b(ctx, &mod->rm);

    int disp_size;
    int32_t disp = 0;

    switch (mod->mod) {
    case 1: {
        disp_size = 1;
        break;
    }
    case 2: {
        disp_size = 4;
        break;
    }
    default:
        return false;
    }

    if (!check_bounds(ctx, disp_size)) {
        printf("not enough bytes for disp\n");
        return false;
    }

    memcpy(&disp, ctx->current, disp_size);
    ctx->current += disp_size;

    if (disp_size == 1)
        disp = (int8_t)disp;

    init_mem_operand(mem_op, mod->rm, REG_NONE, FACTOR_1, disp, addr_size,
        SEG_NONE, op_size);
    return true;
}

static bool handle_instr_pop_reg(
    disasm_ctx_t *ctx, uint8_t opcode, air_instr_t *out)
{
    uint8_t reg = opcode - 0x58;
    extend_reg_with_rex_b(ctx, &reg);
    out->type = AIR_POP;
    init_reg_operand(
        &out->ops.unary.operand, reg, get_reg_size(ctx, REG_SIZE_64));
    return true;
}

static bool handle_instr_pop_seg(
    disasm_ctx_t *ctx, uint8_t opcode, air_instr_t *out)
{
    seg_id_t seg = SEG_NONE;

    switch (opcode) {
    case 0x0f: {
        if (!check_bounds(ctx, 1)) {
            printf("no second byte for 2byte pop\n");
            return false;
        }
        switch (*ctx->current++) {
        case 0xa1: {
            seg = SEG_FS;
            break;
        }
        case 0xa9: {
            seg = SEG_GS;
            break;
        }
        default: {
            printf("pop invalid second byte\n");
            return false;
        }
        }
        break;
    }
    case 0x1f: {
        seg = SEG_DS;
        break;
    }
    case 0x07: {
        seg = SEG_ES;
        break;
    }
    case 0x17: {
        seg = SEG_SS;
        break;
    }
    }

    out->type = AIR_POP;
    init_mem_operand(&out->ops.unary.operand, REG_NONE, REG_NONE, FACTOR_1, 0,
        0, seg, get_operand_size(ctx, OPERAND_SIZE_NONE));
    return true;
}

static bool handle_instr_pop_rm(disasm_ctx_t *ctx, air_instr_t *out)
{
    if (!check_bounds(ctx, 1)) {
        printf("malformed. no modrm byte\n");
        return false;
    }
    struct modrm mod;
    modrm_extract(*ctx->current++, &mod);
    out->type = AIR_POP;
    return handle_memory_operand(ctx, &mod, &out->ops.unary.operand,
        get_operand_size(ctx, OPERAND_SIZE_64));
}

static bool handle_instr_push_reg(
    disasm_ctx_t *ctx, uint8_t opcode, air_instr_t *out)
{
    uint8_t reg = opcode - 0x50;
    extend_reg_with_rex_b(ctx, &reg);
    out->type = AIR_PUSH;
    init_reg_operand(
        &out->ops.unary.operand, reg, get_reg_size(ctx, REG_SIZE_64));
    return true;
}

static bool handle_instr_mov_rm_r(disasm_ctx_t *ctx, air_instr_t *out)
{
    if (!check_bounds(ctx, 1)) {
        printf("malformed. no modrm byte\n");
        return false;
    }

    out->type = AIR_MOV;

    struct modrm mod;
    modrm_extract(*ctx->current++, &mod);
    extend_reg_with_rex_r(ctx, &mod.reg);

    reg_size_t reg_size = get_reg_size(ctx, REG_SIZE_NONE);
    init_reg_operand(&out->ops.binary.src, mod.reg, reg_size);

    if (mod.mod == 3) {
        extend_reg_with_rex_b(ctx, &mod.rm);
        init_reg_operand(&out->ops.binary.dst, mod.rm, reg_size);
        return true;
    }

    return handle_memory_operand(
        ctx, &mod, &out->ops.binary.dst, (operand_size_t)reg_size);
}

void disasm(const uint8_t *instructions, size_t len, air_instr_list_t *out)
{
    disasm_ctx_t ctx = {0};
    ctx.start = instructions;
    ctx.current = instructions;
    ctx.end = instructions + len;

    // TODO: arena allocator or a dynamic growing array
    air_instr_t *instr = (air_instr_t *)malloc(sizeof(*instr));
    if (!instr) {
        printf("failed to allocate instr\n");
        return;
    }

    while (ctx.current < ctx.end) {
        const uint8_t *instr_start = ctx.current;
        disasm_parse_prefixes(&ctx);
        if (ctx.current >= ctx.end)
            break;

        memset(instr, 0, sizeof(*instr));

        uint8_t opcode = *ctx.current++;
        instr_type_t type = instruction_types[opcode];

        bool ok = true;

        switch (type) {
        case INSTR_POP_SEG: {
            ok = handle_instr_pop_seg(&ctx, opcode, instr);
            break;
        }
        case INSTR_POP_REG: {
            ok = handle_instr_pop_reg(&ctx, opcode, instr);
            break;
        }
        case INSTR_POP_RM: {
            ok = handle_instr_pop_rm(&ctx, instr);
            break;
        }
        case INSTR_PUSH_REG: {
            ok = handle_instr_push_reg(&ctx, opcode, instr);
            break;
        }
        case INSTR_MOV_RM_R: {
            ok = handle_instr_mov_rm_r(&ctx, instr);
            break;
        }
        default: {
            printf("skipping unhandled opcode: 0x%02x\n", opcode);
            ok = false;
            break;
        }
        }

        if (ok) {
            instr->length = ctx.current - instr_start;
            air_instr_list_add(out, instr);
            instr = (air_instr_t *)malloc(sizeof(*instr));
            if (!instr) {
                printf("failed to allocate instr\n");
                break;
            }
        }
        reset_ctx(&ctx);
    }

    if (instr) {
        free(instr);
    }
}
