#include "disasm.h"
#include "air.h"
#include "defs.h"
#include "modrm.h"
#include "prefix.h"
#include "sib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

operand_size_t get_operand_size(disasm_ctx_t *ctx)
{
    if (ctx->has_rex && ctx->rex.w)
        return OPERAND_SIZE_64;
    if (HAS_FLAG(ctx->prefixes, INSTR_PREFIX_OP))
        return OPERAND_SIZE_16;
    return OPERAND_SIZE_32;
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
    seg_id_t seg)
{
    op->type = OPERAND_MEM;
    op->mem.base = base;
    op->mem.index = index;
    op->mem.factor = factor;
    op->mem.disp = disp;
    op->mem.size = size;
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
    air_operand_t *mem_op, addr_size_t addr_size)
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
    init_mem_operand(
        mem_op, base_reg, index_reg, s.factor, disp, addr_size, SEG_NONE);
    return true;
}

static bool handle_memory_operand(
    disasm_ctx_t *ctx, struct modrm *mod, air_operand_t *mem_op)
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
            init_mem_operand(
                mem_op, mod->rm, REG_NONE, FACTOR_1, 0, addr_size, SEG_NONE);
            return true;
        }
        case 4: {
            return handle_sib_operand(ctx, mod, mem_op, addr_size);
        }
        case 5: {
            if (!check_bounds(ctx, 4)) {
                printf("not enough bytes for 4byte disp\n");
                return false;
            }

            int32_t disp = get_disp32(ctx);
            init_mem_operand(
                mem_op, REG_IP, REG_NONE, FACTOR_1, disp, addr_size, SEG_NONE);
            return true;
        }
        }
    }

    if (mod->rm == REG_SP) {
        return handle_sib_operand(ctx, mod, mem_op, addr_size);
    }

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

    init_mem_operand(
        mem_op, mod->rm, REG_NONE, FACTOR_1, disp, addr_size, SEG_NONE);
    return true;
}

static bool handle_instr_pop_reg(
    disasm_ctx_t *ctx, uint8_t opcode, air_instr_t *out)
{
    uint8_t reg = opcode - 0x58;
    extend_reg_with_rex_b(ctx, &reg);
    out->type = AIR_POP;
    reg_size_t reg_size =
        HAS_FLAG(ctx->prefixes, INSTR_PREFIX_OP) ? REG_SIZE_16 : REG_SIZE_64;
    init_reg_operand(&out->ops.unary.operand, reg, reg_size);
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
    init_mem_operand(
        &out->ops.unary.operand, REG_NONE, REG_NONE, FACTOR_1, 0, 0, seg);
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
    extend_reg_with_rex_b(ctx, &mod.rm);
    out->type = AIR_POP;
    return handle_memory_operand(ctx, &mod, &out->ops.unary.operand);
}

static bool handle_instr_push_reg(
    disasm_ctx_t *ctx, uint8_t opcode, air_instr_t *out)
{
    uint8_t reg = opcode - 0x50;
    extend_reg_with_rex_b(ctx, &reg);
    out->type = AIR_PUSH;
    reg_size_t reg_size =
        HAS_FLAG(ctx->prefixes, INSTR_PREFIX_OP) ? REG_SIZE_16 : REG_SIZE_64;
    init_reg_operand(&out->ops.unary.operand, reg, reg_size);
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
    
    // TODO: extend more carefully in case we encounter SIB
    extend_reg_with_rex_r(ctx, &mod.reg);
    extend_reg_with_rex_b(ctx, &mod.rm);

    reg_size_t reg_size = (reg_size_t)get_operand_size(ctx);
    init_reg_operand(&out->ops.binary.src, mod.reg, reg_size);

    if (mod.mod == 3) {
        init_reg_operand(&out->ops.binary.dst, mod.rm, reg_size);
        return true;
    }

    return handle_memory_operand(ctx, &mod, &out->ops.binary.dst);
}

static void print_operand(const air_operand_t *op, reg_size_t size_hint)
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

static void print_instr(const air_instr_t *instr)
{
    switch (instr->type) {
    case AIR_POP: {
        const air_operand_t *op = &instr->ops.unary.operand;
        printf("pop ");
        if (op->type == OPERAND_REG) {
            print_operand(op, op->reg.size);
        }
        else if (op->type == OPERAND_MEM) {
            print_operand(op, REG_SIZE_64);
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

void disasm(const uint8_t *instructions, size_t len)
{
    disasm_ctx_t ctx = {0};
    ctx.start = instructions;
    ctx.current = instructions;
    ctx.end = instructions + len;

    air_instr_list_t instr_list = {0};

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
            air_instr_list_add(&instr_list, instr);
            instr = (air_instr_t *)malloc(sizeof(*instr));
            if (!instr) {
                printf("failed to allocate instr\n");
                break;
            }
        }
        reset_ctx(&ctx);
    }

    air_instr_t *node = instr_list.head;
    while (node) {
        air_instr_t *next = node->next;
        print_instr(node);
        free(node);
        node = next;
    }

    if (instr) {
        free(instr);
    }
}
