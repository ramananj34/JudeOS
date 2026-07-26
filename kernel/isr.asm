; Interrupt Service Routine (ISR)

[bits 64]
extern exception_handler
global isr_stub_table

; vectors that do not push an error code: push a dummy 0
%macro ISR_NOERR 1
isr_stub_%1:
    push 0
    push %1
    jmp isr_common
%endmacro

; vectors that do push an error code: CPU already pushed it
%macro ISR_ERR 1
isr_stub_%1:
    push %1
    jmp isr_common
%endmacro

isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi, rsp ; arg1 = pointer to the saved register frame
    call exception_handler
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16 ; discard vector + error code
    iretq

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

section .rodata
isr_stub_table:
%assign i 0
%rep 32
    dq isr_stub_ %+ i
%assign i i+1
%endrep

; hardware IRQ stubs
extern irq_dispatch
global irq_stub_table

%macro IRQ 1
irq_stub_%1:
    push %1 ; push IRQ number (0..15)
    jmp irq_common
%endmacro

section .text
irq_common:
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    mov rdi, [rsp + 72] ; the IRQ number the stub pushed
    sub rsp, 8 ; 16-byte align the stack before the C call
    call irq_dispatch
    add rsp, 8
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    add rsp, 8 ; discard the IRQ number
    iretq

%assign i 0
%rep 16
    IRQ i
%assign i i+1
%endrep

section .rodata
irq_stub_table:
%assign i 0
%rep 16
    dq irq_stub_ %+ i
%assign i i+1
%endrep