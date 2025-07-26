#ifndef PREFIX_H
#define PREFIX_H

#include <stdint.h>
#include <stdbool.h>

#define PREFIX_LOCK 0xF0
#define PREFIX_REPNE_REPNZ 0xF2
#define PREFIX_REP_REPE_REPZ 0xF3

#define PREFIX_CS_SEGMENT_OVERRIDE 0x2E
#define PREFIX_SS_SEGMENT_OVERRIDE 0x36
#define PREFIX_DS_SEGMENT_OVERRIDE 0x3E
#define PREFIX_ES_SEGMENT_OVERRIDE 0x26
#define PREFIX_FS_SEGMENT_OVERRIDE 0x64
#define PREFIX_GS_SEGMENT_OVERRIDE 0x65

#define PREFIX_BRANCH_NOT_TAKEN 0x2E
#define PREFIX_BRANCH_TAKEN 0x3E

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
