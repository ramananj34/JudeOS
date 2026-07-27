[bits 64]
global _ustart
_ustart:
    lea rdi, [rel msg] ; arg1 = pointer to message (rip-relative)
    mov rsi, msg_len ; arg2 = length
    mov rax, 1 ; SYS_WRITE
    syscall

    mov rdi, 0xffffffff80000000 ; a KERNEL address -- kernel must reject this
    mov rsi, 32
    mov rax, 1 ; SYS_WRITE
    syscall

    xor rdi, rdi ; exit code 0
    mov rax, 0 ; SYS_EXIT
    syscall
.hang:
    jmp .hang
msg: db "Hello from ring 3, via a system call!", 10
msg_len equ $ - msg