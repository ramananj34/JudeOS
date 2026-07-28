[bits 64]
global _start
extern main
extern exit
_start:
    call main
    mov edi, eax
    call exit
.hang: jmp .hang