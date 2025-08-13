#include "disasm.h"
#include "air.h"
#include "arch.h"
#include "meta.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch/x86_64/meta.h"

static arch_metadata_t *metadata_table[] = {[ARCH_X86_64] = &x86_64_meta};

void disasm(
    const uint8_t *instructions, size_t len, arch_t arch, air_instr_list_t *out)
{
    disasm_ctx_t ctx = {0};
    ctx.start = instructions;
    ctx.current = instructions;
    ctx.end = instructions + len;

    arch_metadata_t *meta = metadata_table[arch];

    while (ctx.current < ctx.end) {
        const uint8_t *instr_start = ctx.current;
        meta->init_instr(&ctx);
        if (ctx.current >= ctx.end) {
            break;
        }

        uint8_t opcode = *ctx.current++;

        air_instr_t *instr = air_instr_list_get_new(out);
        if (!instr) {
            printf("out of memory\n");
            break;
        }

        bool ok = meta->handle_instr(&ctx, opcode, instr);

        if (ok) {
            instr->length = ctx.current - instr_start;
        }
        else {
            out->count--;
            out->used_in_tail--;
        }

        reset_ctx(&ctx);
    }
}
