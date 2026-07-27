[bits 64]
global enter_user
; void enter_user(uint64_t entry /* rdi */, uint64_t stack /* rsi */)
enter_user:
    mov ax, 0x1b ; user data selector (index 3, RPL 3)
    mov ds, ax
    mov es, ax
    push 0x1b ; SS  = user data
    push rsi ; RSP = user stack
    push 0x2 ; RFLAGS (IF off for a deterministic demo)
    push 0x23 ; CS  = user code (index 4, RPL 3)
    push rdi ; RIP = entry
    iretq ; drop to ring 3