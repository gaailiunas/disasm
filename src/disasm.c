#include "disasm.h"
#include "modrm.h"
#include "prefix.h"
#include <stdio.h>

void disasm(const uint8_t *instructions, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        switch (instructions[i]) {
            case 0x50 ... 0x57: {
                bool x64 = true;
                uint8_t reg = instructions[i] - 0x50;
                if (i > 0) {
                    struct rex_prefix rex;
                    uint8_t prefix = instructions[i - 1]; 
                    if (rex_extract(prefix, &rex)) {
                        if (rex.b) {
                            reg += 8;
                        }
                        x64 = rex.w;
                    }
                }

                const char *reg_name = x64 ? reg_names_x64[reg] : reg_names_x86[reg];
                printf("push %s\n", reg_name);
                break;
            }
            case 0x89: {
                /*
                 * MOV 	r/m16/32/64 	r16/32/64
                 */
                

                if (i + 1 >= len) {
                    printf("malformed. no modrm byte\n");
                    return;
                }
                
                struct modrm mod;
                modrm_extract(instructions[i + 1], &mod);

                bool x64 = false;

                if (i > 0) {
                    struct rex_prefix rex;
                    uint8_t prefix = instructions[i - 1]; 
                    if (rex_extract(prefix, &rex)) {
                        if (rex.r) {
                            mod.reg += 8;
                        }
                        if (rex.b) {
                            mod.rm += 8;
                        }
                        x64 = rex.w;
                    }
                }

                if (mod.mod == 3) {
                    // register-direct addressing mode is used
                    uint8_t src_reg = mod.reg;
                    uint8_t dst_reg = mod.rm;

                    const char *src_name = x64 ? reg_names_x64[src_reg] : reg_names_x86[src_reg];
                    const char *dst_name = x64 ? reg_names_x64[dst_reg] : reg_names_x86[dst_reg];

                    printf("mov %s, %s\n", src_name, dst_name);
                }
                else {
                    printf("mov with memory operand is not yet implemented\n");
                }

                i += 1;
                break;
            }
        }
    }
}
