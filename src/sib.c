#include "sib.h"

void sib_extract(uint8_t sib, struct sib *out)
{
    out->base = sib & 0x7;
    out->index = (sib >> 3) & 0x7;
    out->scale = sib >> 6;
    switch (out->scale) {
    case 0: {
        out->factor = FACTOR_1;
        break;
    }
    case 1: {
        out->factor = FACTOR_2;
        break;
    }
    case 2: {
        out->factor = FACTOR_4;
        break;
    }
    case 3: {
        out->factor = FACTOR_8;
        break;
    }
    }
}
