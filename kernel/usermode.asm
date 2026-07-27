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

default rel
extern syscall_dispatch
extern syscall_kstack
extern saved_user_rsp
global syscall_entry
; syscall lands here: rcx=return rip, r11=return rflags, still on the USER stack
syscall_entry:
    mov [saved_user_rsp], rsp ; stash user rsp
    mov rsp, [syscall_kstack] ; switch to a kernel stack
    push rcx ; save return rip
    push r11 ; save return rflags
    ; shuffle to C ABI: dispatch(num=rax, a1=rdi, a2=rsi, a3=rdx)
    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, rax
    call syscall_dispatch
    pop r11 ; restore return rflags
    pop rcx ; restore return rip
    mov rsp, [saved_user_rsp] ; restore user rsp
    o64 sysret ; back to ring 3