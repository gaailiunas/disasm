#include "disasm.h"
#include "modrm.h"
#include "prefix.h"
#include "sib.h"
#include <string.h>
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

                printf("mod rm: %d\n", mod.rm);

                bool rex_present = false;
                bool x64 = false;
                bool addrsize_override = false;

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
                        rex_present = true;
                        x64 = rex.w;
                    }

                    if (i > 1 && instructions[i - 2] == PREFIX_ADDR_SIZE_OVERRIDE) {
                        addrsize_override = true;
                    }
                }

                if (mod.mod == 3) {
                    // register-direct addressing mode is used
                    uint8_t src_reg = mod.reg;
                    uint8_t dst_reg = mod.rm;

                    const char *src_name = x64 ? reg_names_x64[src_reg] : reg_names_x86[src_reg];
                    const char *dst_name = x64 ? reg_names_x64[dst_reg] : reg_names_x86[dst_reg];

                    printf("mov %s, %s\n", dst_name, src_name);
                    i += 1;
                    continue;
                }
                else {
                    // register-indirect addressing mode is used
                    if (mod.mod == 0) {
                        if ((mod.rm >= 0 && mod.rm <= 3) || mod.rm == 6 || mod.rm == 7) {
                            uint8_t src_reg = mod.reg;
                            uint8_t dst_reg = mod.rm;

                            printf("mov [%s], %s\n", reg_names_x64[dst_reg], reg_names_x64[src_reg]);
                            i += 1;
                            continue;
                        }

                        // handle SIB
                        if (mod.rm == 4) {
                            printf("here\n");
                            if (i + 2 >= len) {
                                printf("no sib byte\n");
                                return;
                            }
                         
                            struct sib s;
                            sib_extract(instructions[i + 2], &s);

                            printf("mov [%s+%s*%d], %s\n", reg_names_x64[s.base], reg_names_x64[s.index], s.factor, reg_names_x64[mod.reg]);

                            i += 2;
                            continue;
                        }

                        // handle [RIP/EIP + disp32]
                        if (mod.rm == 5) {
                            uint32_t disp;
                            memcpy(&disp, &instructions[i + 2], 4);

                            uint32_t offset = 6;
                            if (addrsize_override)
                                offset++;
                            if (rex_present)
                                offset++;
                            offset += disp;
                            
                            const char *reg = addrsize_override ? "eip" : "rip";
                            printf("mov [%s+0x%x], %s # %x\n", reg, disp, reg_names_x64[mod.reg], offset);
                            i += 6;
                            continue;
                        }
                    }
                    else if (mod.mod == 1) {
                    }
                    else if (mod.mod == 2) {
                    }
                }

                break;
            }
        }
    }
}
