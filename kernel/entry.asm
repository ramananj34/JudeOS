; Establish the stack for the C code

[bits 64]
global _start
extern kmain

section .text
_start:
    mov rsp, stack_top ; the kernel's own dedicated stack
    xor rbp, rbp ; clean base pointer (end of call chain)
    ; rdi already holds the boot_info pointer, placed there by the bootloader
    call kmain
.hang:
    hlt
    jmp .hang

section .bss ; bss needs to be handeled seperatley
align 16
stack_bottom:
    resb 16384 ; 16 KiB kernel stack
stack_top: