#include "prefix.h"

bool rex_extract(uint8_t prefix, struct rex_prefix *out)
{
    unsigned char fixed = prefix >> 4;

    // 0b0100
    if (fixed != 4) {
        return false;
    }

    out->b = prefix & 0x1;
    out->x = (prefix >> 1) & 0x1;
    out->r = (prefix >> 2) & 0x1;
    out->w = (prefix >> 3) & 0x1;
    return true;
}
