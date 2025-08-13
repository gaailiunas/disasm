cmake_minimum_required(VERSION 3.10)
project(disasm)
add_executable(disasm
    src/main.c
    src/disasm.c
    src/prefix.c
    src/modrm.c
    src/sib.c
    src/air.c
    src/frontend.c
    src/arch/x86_64/optable.c
    src/arch/x86_64/meta.c
)
