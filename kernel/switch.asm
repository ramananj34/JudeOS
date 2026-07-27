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

; new threads "return" here via context_switch; enable interrupts, then jump to entry (in r15)
global thread_trampoline
thread_trampoline:
    sti
    jmp r15

global process_user_trampoline
process_user_trampoline:
    push 0x1b ; SS  = user data
    push r14 ; user rsp
    push 0x202 ; rflags (IF on)
    push 0x23 ; CS  = user code
    push r15 ; user entry
    iretq