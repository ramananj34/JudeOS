[bits 64]
global context_switch

; void context_switch(uint64_t *old_rsp /* rdi */, uint64_t new_rsp /* rsi */)
; save the current thread's callee-saved regs + rsp, load the next thread's
context_switch:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    mov [rdi], rsp ; save current stack pointer into *old_rsp
    mov rsp, rsi ; switch to the new thread's stack
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret ; "returns" into the new thread's saved context