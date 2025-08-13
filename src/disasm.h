#ifndef DISASM_H
#define DISASM_H

#include "air.h"
#include "arch.h"
#include "context.h"
#include "defs.h"
#include "prefix.h"
#include <stddef.h>
#include <stdint.h>

void disasm(const uint8_t *instructions, size_t len, arch_t arch,
    air_instr_list_t *out);

#endif // DISASM_H
