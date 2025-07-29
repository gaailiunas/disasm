# disasm (x86_64 disassembler)

just for educational purposes, the end goal is to write a decompiler


## Build from source

### Prerequisites
- Cmake 3.10>

### Build executable

```bash
mkdir build
cd build
cmake ..
make
```

- The executable will be in the `build` folder

## Usage

- Print to stdout
```bash
./disasm <executable>
```

- Write it to a file
```bash
./disasm <executable> > disasm.s
```

### Example

```bash
./disasm disasm
push rbp
mov rbp, rsp
mov [rax], rsi
mov esi, eax
mov rdi, rdx
mov [rsp+rax*4], rdi
mov [eax+edx*4], edi
mov [eax+edx*4], rdi
mov [eax+edx*4], di
mov [rax+rdx*4], edi
mov [rax+rdx*4], di
mov [rsi], eax
mov [rsi], rax
mov [rsi], ax
mov [esi], eax
mov [esi], rax
mov [esi], ax
```
- https://youtu.be/o8u7CM8VEOo 
