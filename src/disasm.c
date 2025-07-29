#include "disasm.h"
#include "modrm.h"
#include "prefix.h"
#include "sib.h"
#include <string.h>
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
};

const uint8_t instruction_types[256] = {
    [0x50] = INSTR_PUSH_REG,
    [0x51] = INSTR_PUSH_REG,
    [0x52] = INSTR_PUSH_REG,
    [0x53] = INSTR_PUSH_REG,
    [0x54] = INSTR_PUSH_REG,
    [0x55] = INSTR_PUSH_REG,
    [0x56] = INSTR_PUSH_REG,
    [0x57] = INSTR_PUSH_REG,
    [0x89] = INSTR_MOV_RM_R,
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

const char *get_reg_name(uint8_t reg, reg_size_t size)
{
    if (reg >= sizeof(reg_names) / sizeof(reg_names[0]))
        return "unk";

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

// returns a number of bytes consumed
static size_t handle_memory_operand(disasm_ctx_t *ctx, struct modrm *mod)
{
    size_t consumed = 0;
    
    if (mod->mod == 0) {
        if ((mod->rm >= 0 && mod->rm <= 3) || mod->rm == 6 || mod->rm == 7) {
            uint8_t src = mod->reg;
            uint8_t dst = mod->rm;

            const char *src_reg;
            const char *dst_reg;

            if (ctx->has_rex && ctx->rex.w) {
                if (HAS_FLAG(ctx->prefixes, INSTR_PREFIX_ADDR_SIZE)) {
                    dst_reg = get_reg_name(dst, REG_SIZE_32);
                    src_reg = get_reg_name(src, REG_SIZE_64);
                }
                else {
                    dst_reg = get_reg_name(dst, REG_SIZE_64);
                    src_reg = get_reg_name(src, REG_SIZE_64);
                }
            }
            else if (HAS_FLAG(ctx->prefixes, INSTR_PREFIX_OP)) {
                if (HAS_FLAG(ctx->prefixes, INSTR_PREFIX_ADDR_SIZE)) {
                    dst_reg = get_reg_name(dst, REG_SIZE_32);
                    src_reg = get_reg_name(src, REG_SIZE_16);
                }
                else {
                    dst_reg = get_reg_name(dst, REG_SIZE_64);
                    src_reg = get_reg_name(src, REG_SIZE_16);
                }
            }
            else {
                if (HAS_FLAG(ctx->prefixes, INSTR_PREFIX_ADDR_SIZE)) {
                    dst_reg = get_reg_name(dst, REG_SIZE_32);
                    src_reg = get_reg_name(src, REG_SIZE_32);
                }
                else {
                    dst_reg = get_reg_name(dst, REG_SIZE_64);
                    src_reg = get_reg_name(src, REG_SIZE_32);
                }
            }

            printf("mov [%s], %s\n", dst_reg, src_reg);
        }
        if (mod->rm == 4) {
            if (!check_bounds(ctx, 1)) {
                printf("no sib byte\n");
                return consumed;
            }

            struct sib s;
            sib_extract(*ctx->current, &s);
            consumed++;

            const char *src_reg;
            const char *dst_reg0;
            const char *dst_reg1;

            if (HAS_FLAG(ctx->prefixes, INSTR_PREFIX_ADDR_SIZE)) {
                if (ctx->has_rex && ctx->rex.w) {
                    dst_reg0 = get_reg_name(s.base, REG_SIZE_32);
                    dst_reg1 = get_reg_name(s.index, REG_SIZE_32);
                    src_reg = get_reg_name(mod->reg, REG_SIZE_64);
                }
                else if (HAS_FLAG(ctx->prefixes, INSTR_PREFIX_OP)) {
                    dst_reg0 = get_reg_name(s.base, REG_SIZE_32);
                    dst_reg1 = get_reg_name(s.index, REG_SIZE_32);
                    src_reg = get_reg_name(mod->reg, REG_SIZE_16);
                }
                else {
                    dst_reg0 = get_reg_name(s.base, REG_SIZE_32);
                    dst_reg1 = get_reg_name(s.index, REG_SIZE_32);
                    src_reg = get_reg_name(mod->reg, REG_SIZE_32);
                }
            }
            else if (ctx->has_rex && ctx->rex.w) {
                dst_reg0 = get_reg_name(s.base, REG_SIZE_64);
                dst_reg1 = get_reg_name(s.index, REG_SIZE_64);
                src_reg = get_reg_name(mod->reg, REG_SIZE_64);
            }
            else if (HAS_FLAG(ctx->prefixes, INSTR_PREFIX_OP)) {
                dst_reg0 = get_reg_name(s.base, REG_SIZE_64);
                dst_reg1 = get_reg_name(s.index, REG_SIZE_64);
                src_reg = get_reg_name(mod->reg, REG_SIZE_16);
            }
            else {
                dst_reg0 = get_reg_name(s.base, REG_SIZE_64);
                dst_reg1 = get_reg_name(s.index, REG_SIZE_64);
                src_reg = get_reg_name(mod->reg, REG_SIZE_32);
            }

            printf("mov [%s+%s*%d], %s\n", dst_reg0, dst_reg1, s.factor, src_reg);
        }
    }
    return consumed;
}

void disasm(const uint8_t *instructions, size_t len)
{
    disasm_ctx_t ctx = {0};
    ctx.start = instructions;
    ctx.current = instructions;
    ctx.end = instructions + len;

    while (ctx.current < ctx.end) {
        disasm_parse_prefixes(&ctx);
        if (ctx.current >= ctx.end)
            break;

        uint8_t opcode = *ctx.current;
        //printf("opcode: %d\n", opcode);
        size_t consumed = 1;

        instr_type_t type = instruction_types[opcode];
        switch (type) {
            case INSTR_PUSH_REG: {
                uint8_t reg = opcode - 0x50;
                bool x64 = true;
                if (ctx.has_rex) {
                    if (ctx.rex.b)
                        reg += 8;
                    x64 = ctx.rex.w; 
                }
                const char *reg_name = get_reg_name(reg, x64 ? REG_SIZE_64 : REG_SIZE_32);
                printf("push %s\n", reg_name);
                break;
            }
            case INSTR_MOV_RM_R: {
                if (!check_bounds(&ctx, 1)) {
                    printf("malformed. no modrm byte\n");
                    break;
                }
                
                struct modrm mod;
                modrm_extract(ctx.current[1], &mod);
                consumed++; // modrm

                bool x64 = false;

                // apply extensions
                if (ctx.has_rex) {
                    if (ctx.rex.r)
                        mod.reg += 8;
                    if (ctx.rex.b)
                        mod.rm += 8;
                    x64 = ctx.rex.w;
                }

                if (mod.mod == 3) {
                    const char *src_name = get_reg_name(mod.reg, x64 ? REG_SIZE_64 : REG_SIZE_32);
                    const char *dst_name = get_reg_name(mod.rm, x64 ? REG_SIZE_64 : REG_SIZE_32);
                    printf("mov %s, %s\n", dst_name, src_name);
                }
                else {
                    ctx.current += consumed;
                    consumed = handle_memory_operand(&ctx, &mod);
                }
                break;
            }
            default: {
                //printf("unknown opcode: 0x%02x\n", opcode);
                break;
            }
        }

        ctx.current += consumed;
        reset_ctx(&ctx);
    }
}
