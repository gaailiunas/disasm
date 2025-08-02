#ifndef MODRM_H
#define MODRM_H

#include <stdbool.h>
#include <stdint.h>

struct modrm {
    uint8_t mod;
    uint8_t reg;
    uint8_t rm;
};

void modrm_extract(uint8_t modrm, struct modrm *out);

#endif // MODRM_H
