#ifndef META_H
#define META_H

#include "air.h"
#include "context.h"
#include <stdbool.h>

typedef struct {
    void (*init_instr)(disasm_ctx_t
            *ctx); // parses prefixes and etc before processing the instruction
    bool (*handle_instr)(disasm_ctx_t *ctx, uint8_t opcode, air_instr_t *out);
} arch_metadata_t;

#endif // META_H
