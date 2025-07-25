#ifndef PREFIX_H
#define PREFIX_H

#include <stdint.h>
#include <stdbool.h>

/*
 * mandatory prefixes:
 * 0x66, 0xF2 or 0xF3
 */

struct rex_prefix {
    bool w;
    bool r;
    bool x;
    bool b;
};

bool rex_extract(uint8_t prefix, struct rex_prefix *rex);

#endif // PREFIX_H
