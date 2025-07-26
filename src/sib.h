#ifndef SIB_H
#define SIB_H

#include <stdint.h>

struct sib {
    uint8_t scale;
    uint8_t index;
    uint8_t base;
    uint8_t factor;
};

void sib_extract(uint8_t sib, struct sib *out);

#endif // SIB_H
