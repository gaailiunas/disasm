#ifndef PREFIX_H
#define PREFIX_H

#include <stdbool.h>
#include <stdint.h>

#define PREFIX_LOCK 0xF0
#define PREFIX_REPNE 0xF2
#define PREFIX_REP_REPE 0xF3

#define PREFIX_0x2e 0x2e
#define PREFIX_0x3e 0x3e

#define PREFIX_SEG_CS PREFIX_0x2e
#define PREFIX_SEG_SS 0x36
#define PREFIX_SEG_DS PREFIX_0x3e
#define PREFIX_SEG_ES 0x26
#define PREFIX_SEG_FS 0x64
#define PREFIX_SEG_GS 0x65

#define PREFIX_BRANCH_NOT_TAKEN PREFIX_0x2e
#define PREFIX_BRANCH_TAKEN PREFIX_0x3e

#define PREFIX_OP_SIZE_OVERRIDE 0x66
#define PREFIX_ADDR_SIZE_OVERRIDE 0x67

struct rex_prefix {
    bool w;
    bool r;
    bool x;
    bool b;
};

bool rex_extract(uint8_t prefix, struct rex_prefix *out);

#endif // PREFIX_H
