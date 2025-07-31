#include "disasm.h"
#include "air.h"
#include "defs.h"
#include "modrm.h"
#include "prefix.h"
#include "sib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

const uint8_t instruction_types[256] = {
    [0x07] = INSTR_POP_SEGREG,
    [0x0F] = INSTR_POP_SEGREG,
    [0x17] = INSTR_POP_SEGREG,
    [0x1F] = INSTR_POP_SEGREG,

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
    return HAS_FLAG(ctx->prefixes, INSTR_PREFIX_ADDR_SIZE) ? ADDR_SIZE_32 : ADDR_SIZE_64;
}

operand_size_t get_operand_size(disasm_ctx_t *ctx)
{
    if (ctx->has_rex && ctx->rex.w)
        return OPERAND_SIZE_64;
    if (HAS_FLAG(ctx->prefixes, INSTR_PREFIX_OP))
        return OPERAND_SIZE_16;
    return OPERAND_SIZE_32;
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
    if (reg >= sizeof(reg_names) / sizeof(reg_names[0])) {
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

static inline void init_reg_operand(air_operand_t *op, reg_id_t reg, reg_size_t size)
{
    op->type = OPERAND_REG;
    op->reg.id = reg;
    op->reg.size = size;
}

static inline void init_mem_operand(air_operand_t *op, reg_id_t base, reg_id_t index, 
    scale_factor_t factor, int32_t disp, addr_size_t size)
{
    op->type = OPERAND_MEM;
    op->mem.base = base;
    op->mem.index = index;
    op->mem.factor = factor;
    op->mem.disp = disp;
    op->mem.size = size;
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

static bool handle_memory_operand(disasm_ctx_t *ctx, struct modrm *mod, air_instr_t *out)
{
    operand_size_t op_size = get_operand_size(ctx);
    addr_size_t addr_size = get_addr_size(ctx);
    
    // casting for convenience, avoiding a bunch of switch cases
    reg_size_t reg_op_size = (reg_size_t)op_size;
    reg_size_t reg_addr_size = (reg_size_t)addr_size;

    if (mod->mod == 0) {
        switch (mod->rm) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 6:
            case 7: {
                init_reg_operand(&out->ops.binary.src, mod->reg, reg_op_size);
                init_mem_operand(&out->ops.binary.dst, mod->rm, REG_NONE, FACTOR_1, 0, addr_size);
                return true;
            }
            case 4: {
                if (!check_bounds(ctx, 1)) {
                    printf("no sib byte\n");
                    return false;
                }

                /* TODO: put it all in a function to handle SIB */

                struct sib s;
                sib_extract(*ctx->current++, &s);
                extend_reg_with_rex_x(ctx, &s.index);

                init_reg_operand(&out->ops.binary.src, mod->reg, reg_op_size);

                if (s.base == REG_BP || s.base == REG_R13) {
                    if (!check_bounds(ctx, 4)) {
                        printf("not enough bytes for 4byte disp\n");
                        return false;
                    }

                    int32_t disp = get_disp32(ctx);

                    if (s.index == REG_SP) {
                        init_mem_operand(&out->ops.binary.dst, REG_NONE, REG_NONE, FACTOR_1, disp, addr_size);
                    }
                    else {
                        init_mem_operand(&out->ops.binary.dst, REG_NONE, s.index, s.factor, disp, addr_size);
                    }
                }
                else {
                    init_mem_operand(&out->ops.binary.dst, s.base, s.index, s.factor, 0, addr_size);
                }

                return true;
            }
            case 5: {
                if (!check_bounds(ctx, 4)) {
                    printf("not enough bytes for 4byte disp\n");
                    return false;
                }

                int32_t disp = get_disp32(ctx);
               
                init_reg_operand(&out->ops.binary.src, mod->reg, reg_op_size);
                init_mem_operand(&out->ops.binary.dst, REG_IP, REG_NONE, FACTOR_1, disp, addr_size);
                return true;
            }
        }
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

    init_reg_operand(&out->ops.binary.src, mod->reg, reg_op_size);

    if (mod->rm == REG_SP) {
        if (!check_bounds(ctx, 1 + disp_size)) {
            printf("no sib byte or sib+disp\n");
            return false;
        }
        
        struct sib s;
        sib_extract(*ctx->current++, &s);

        memcpy(&disp, ctx->current, disp_size);
        ctx->current += disp_size;

        if (disp_size == 1)
            disp = (int8_t)disp;

        if (s.index == REG_SP) {
            init_mem_operand(&out->ops.binary.dst, s.base, REG_NONE, FACTOR_1, disp, addr_size);
        }
        else {
            init_mem_operand(&out->ops.binary.dst, s.base, s.index, s.factor, disp, addr_size);
        }
    }
    else {
        if (!check_bounds(ctx, disp_size)) {
            printf("not enough bytes for disp\n");
            return false;
        }

        memcpy(&disp, ctx->current, disp_size);
        ctx->current += disp_size;

        if (disp_size == 1)
            disp = (int8_t)disp;

        init_mem_operand(&out->ops.binary.dst, mod->rm, REG_NONE, FACTOR_1, disp, addr_size);
    }

    return true;
}

static bool handle_instr_pop_reg(disasm_ctx_t *ctx, uint8_t opcode, air_instr_t *instr)
{
    instr->type = AIR_POP;
    uint8_t reg = opcode - 0x58;

    extend_reg_with_rex_b(ctx, &reg);
    reg_size_t reg_size = HAS_FLAG(ctx->prefixes, INSTR_PREFIX_OP) ? REG_SIZE_16 : REG_SIZE_64;

    instr->ops.unary.operand.type = OPERAND_REG;
    instr->ops.unary.operand.reg.id = reg;
    instr->ops.unary.operand.reg.size = reg_size;
    return true;
}

static bool handle_instr_pop_segreg(disasm_ctx_t *ctx, uint8_t opcode, air_instr_t *instr)
{
    uint8_t seg = SEGMENT_NONE;

    instr->type = AIR_POP;
    instr->ops.unary.operand.type = OPERAND_SEGMENT;

    if (opcode == 0x1F) {
        seg = SEGMENT_DS;
    } else if (opcode == 0x07) {
        seg = SEGMENT_ES;
    } else if (opcode == 0x17) {
        seg = SEGMENT_SS;
    } else {
        switch (*(ctx->current++)) {
            case 0xA1: {
                seg = SEGMENT_FS;
                break;
            }
            case 0xA9: {
                seg = SEGMENT_GS;
                break;
            }
        }
    }

    instr->ops.unary.operand.segment.id = seg;
    return true;
}

static bool handle_pop_rm(disasm_ctx_t *ctx, air_instr_t *out)
{
    if (!check_bounds(ctx, 1)) {
        printf("malformed. no modrm byte\n");
        return NULL;
    }
  
    out->type = AIR_POP;
    
    struct modrm mod;
    modrm_extract(*ctx->current++, &mod);
    extend_reg_with_rex_b(ctx, &mod.rm);
    reg_size_t reg_size = HAS_FLAG(ctx->prefixes, INSTR_PREFIX_OP) ? REG_SIZE_16 : REG_SIZE_64;

    out->ops.unary.operand.type = OPERAND_REG;
    out->ops.unary.operand.reg.id = mod.rm;
    out->ops.unary.operand.reg.size = reg_size;

    return handle_memory_operand(ctx, &mod, out);
}


static bool handle_instr_push_reg(disasm_ctx_t *ctx, uint8_t opcode, air_instr_t *instr)
{
    uint8_t reg = opcode - 0x50;
    extend_reg_with_rex_b(ctx, &reg);
    reg_size_t reg_size = HAS_FLAG(ctx->prefixes, INSTR_PREFIX_OP) ? REG_SIZE_16 : REG_SIZE_64;

    instr->type = AIR_PUSH;
    init_reg_operand(&instr->ops.unary.operand, reg, reg_size);
    return true;
}

static bool handle_mov_rm_r(disasm_ctx_t *ctx, air_instr_t *out)
{
    if (!check_bounds(ctx, 1)) {
        printf("malformed. no modrm byte\n");
        return false;
    }
  
    out->type = AIR_MOV;
    
    struct modrm mod;
    modrm_extract(*ctx->current++, &mod);

    extend_reg_with_rex_r(ctx, &mod.reg);
    extend_reg_with_rex_b(ctx, &mod.rm);

    if (mod.mod == 3) {
        operand_size_t op_size = get_operand_size(ctx);
        reg_size_t reg_size = (reg_size_t)op_size;

        init_reg_operand(&out->ops.binary.src, mod.reg, reg_size);
        init_reg_operand(&out->ops.binary.dst, mod.rm, reg_size);
        return true;
    }

    return handle_memory_operand(ctx, &mod, out);
}

static void print_segment(const air_operand_t *op)
{
    if (op->type == OPERAND_SEGMENT) {
        switch (op->segment.id) {
            case SEGMENT_ES: {
                printf("es");
                break;
            }
            case SEGMENT_CS: {
                printf("cs");
                break;
            }
            case SEGMENT_SS: {
                printf("ss");
                break;
            }
            case SEGMENT_DS: {
                printf("ds");
                break;
            }
            case SEGMENT_FS: {
                printf("fs");
                break;
            }
            case SEGMENT_GS: {
                printf("gs");
                break;
            }
            default: {
                printf("???");
                break;
            }
        }
    }
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
            const char *size_str = op_size_suffixes[size_hint];
            printf("%s ptr [", size_str);

            bool need_plus = false;

            if (op->mem.base != REG_NONE) {
                printf("%s", get_reg_name(op->mem.base, (reg_size_t)op->mem.size));
                need_plus = true;
            }

            if (op->mem.index != REG_NONE) {
                if (need_plus) printf("+");
                printf("%s*%d", get_reg_name(op->mem.index, (reg_size_t)op->mem.size), op->mem.factor);
                need_plus = true;
            }

            int32_t disp = op->mem.disp;

            if (disp != 0 || (!need_plus && op->mem.base == REG_NONE && op->mem.index == REG_NONE)) {
                if (disp < 0) {
                    printf("-%#x", -disp);
                } else {
                    if (need_plus) printf("+");
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
            } else if (op->type == OPERAND_MEM) {
                print_operand(op, REG_SIZE_64);
            } else if (op->type == OPERAND_SEGMENT) {
                print_segment(op);
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
            case INSTR_POP_SEGREG: {
                ok = handle_instr_pop_segreg(&ctx, opcode, instr);
                break;
            }
            case INSTR_POP_REG: {
                ok = handle_instr_pop_reg(&ctx, opcode, instr);
                break;
            }
            case INSTR_POP_RM: {
                ok = handle_pop_rm(&ctx, instr);
                break;
            }
            case INSTR_PUSH_REG: {
                ok = handle_instr_push_reg(&ctx, opcode, instr);
                break;
            }
            case INSTR_MOV_RM_R: {
                ok = handle_mov_rm_r(&ctx, instr);
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
