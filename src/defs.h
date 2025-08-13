#ifndef DEFS_H
#define DEFS_H

typedef enum {
    /* group 1 */
    INSTR_PREFIX_LOCK = 1 << 0,
    INSTR_PREFIX_REPNE = 1 << 1,
    INSTR_PREFIX_REP_REPE = 1 << 2,
    /* group 2 */
    INSTR_PREFIX_0x2e = 1 << 3,
    INSTR_PREFIX_SS = 1 << 4,
    INSTR_PREFIX_0x3e = 1 << 5,
    INSTR_PREFIX_ES = 1 << 6,
    INSTR_PREFIX_FS = 1 << 7,
    INSTR_PREFIX_GS = 1 << 8,
    INSTR_PREFIX_BRANCH_NOT_TAKEN = 1 << 9,
    INSTR_PREFIX_BRANCH_TAKEN = 1 << 10,
    /* group 3 */
    INSTR_PREFIX_OP = 1 << 11,
    /* group 4 */
    INSTR_PREFIX_ADDR_SIZE = 1 << 12,
} instr_prefix_flag_t;

typedef enum {
    REG_SIZE_NONE = -1,

    REG_SIZE_16 = 0,
    REG_SIZE_32 = 1,
    REG_SIZE_64 = 2,
} reg_size_t;

typedef enum {
    ADDR_SIZE_NONE = -1,

    ADDR_SIZE_32 = 1,
    ADDR_SIZE_64 = 2,
} addr_size_t;

typedef enum {
    OPERAND_SIZE_NONE = -1,

    OPERAND_SIZE_16 = 0,
    OPERAND_SIZE_32 = 1,
    OPERAND_SIZE_64 = 2,
} operand_size_t;

typedef enum {
    REG_AX,
    REG_CX,
    REG_DX,
    REG_BX,
    REG_SP,
    REG_BP,
    REG_SI,
    REG_DI,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
    REG_IP, // on actual hardware, IP doesn't have an id

    REG_NONE = 0xff,
} reg_id_t;

typedef enum {
    SEG_ES,
    SEG_CS,
    SEG_SS,
    SEG_DS,
    SEG_FS,
    SEG_GS,

    SEG_NONE = 0xff,
} seg_id_t;

#endif // DEFS_H
