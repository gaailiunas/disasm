#include "disasm.h"
#include "prefix.h"
#include <stdio.h>

void disasm(const uint8_t *instructions, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        switch (instructions[i]) {
            case 0x50 ... 0x57: {
                uint8_t reg = instructions[i] - 0x50;
                if (i - 1 >= 0) {
                    struct rex_prefix rex;
                    uint8_t prefix = instructions[i - 1]; 
                    bool is_rex = rex_extract(prefix, &rex);
                    if (is_rex && rex.b) {
                        reg += 8;
                    }
                }
                printf("push %s\n", reg_names[reg]);
            }
        }
    }
}
