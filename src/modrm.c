#include "modrm.h"

void modrm_extract(uint8_t modrm, struct modrm *out)
{
    out->rm = modrm & 0x7;
    out->reg = (modrm >> 3) & 0x7;
    out->mod = modrm >> 6;
}
