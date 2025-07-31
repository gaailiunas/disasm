#ifndef SIB_H
#define SIB_H

#include <stdint.h>

typedef enum {
    FACTOR_1 = 1,
    FACTOR_2 = 2, 
    FACTOR_4 = 4,
    FACTOR_8 = 8
} scale_factor_t;

struct sib {
    uint8_t scale;
    uint8_t index;
    uint8_t base;
    scale_factor_t factor;
};

void sib_extract(uint8_t sib, struct sib *out);

#endif // SIB_H
