#include "prefix.h"

bool rex_extract(uint8_t prefix, struct rex_prefix *rex)
{
    unsigned char fixed = prefix >> 4;

    // 0b0100
    if (fixed != 4) {
        return false;
    }

    rex->b = prefix & 0x1;
    rex->x = (prefix >> 1) & 0x1;
    rex->r = (prefix >> 2) & 0x1;
    rex->w = (prefix >> 3) & 0x1;
    return true;
}
