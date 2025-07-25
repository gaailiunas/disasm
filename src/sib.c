#include "sib.h"

void sib_extract(uint8_t sib, struct sib *out)
{
    out->base = sib & 0x7;
    out->index = (sib >> 3) & 0x7;
    out->scale = sib >> 6;
}
