#ifndef DISASM_H
#define DISASM_H

#include <stdint.h>
#include <stddef.h>

enum disasm_register {
    REG_RAX,
    REG_RCX,
    REG_RDX,
    REG_RBX,
    REG_RSP,
    REG_RBP,
    REG_RSI,
    REG_RDI,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
};

static const char *reg_names[] = {
    "rax",
    "rcx",
    "rdx",
    "rbx",
    "rsp",
    "rbp",
    "rsi",
    "rdi",
    "r8", 
    "r9", 
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15" 
};

// TODO: impl a linked list for storing all asm 
void disasm(const uint8_t *instructions, size_t len);

#endif // DISASM_H
