[bits 64]
global _ustart
_ustart:
    mov rax, 2 ; SYS_GETPID
    syscall
    mov r15, rax ; r15 = my pid
    mov r14, 8 ; iterations
.loop:
    mov [rel myvar], r15 ; write my pid into MY data page
    mov rcx, 3000000 ; delay so the timer preempts us mid-work
.d: dec rcx
    jnz .d
    mov rbx, [rel myvar] ; read it back
    mov al, '!' ; '!' means isolation was violated
    cmp rbx, r15
    jne .emit
    lea rax, [r15 + 0x40] ; pid 1 -> 'A', pid 2 -> 'B'
.emit:
    mov [rel mychar], al
    lea rdi, [rel mychar]
    mov rsi, 1
    mov rax, 1 ; SYS_WRITE
    syscall
    dec r14
    jnz .loop
    xor rdi, rdi
    mov rax, 0 ; SYS_EXIT
    syscall
.hang: jmp .hang
section .data
myvar: dq 0
mychar: db 0