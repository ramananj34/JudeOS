//Interrupt Descriptor Table is 256 slots.
//Slot N tells the CPU where to jump when interrupt/exception N fires.

#include "idt.h"
#include "console.h"
#include <stdint.h>

struct __attribute__((packed)) idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
};

static struct idt_entry idt[256];
static struct __attribute__((packed)) { uint16_t limit; uint64_t base; } idtr;

extern void *isr_stub_table[];

struct regs {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

static const char *names[32] = {
    "Divide Error","Debug","NMI","Breakpoint","Overflow","BOUND Range",
    "Invalid Opcode","Device Not Available","Double Fault","Coprocessor",
    "Invalid TSS","Segment Not Present","Stack Fault","General Protection",
    "Page Fault","Reserved","x87 FP","Alignment Check","Machine Check",
    "SIMD FP","Virtualization","Control Protection","Reserved","Reserved",
    "Reserved","Reserved","Reserved","Reserved","Hypervisor","VMM Comm",
    "Security","Reserved"
};

void exception_handler(struct regs *r) {
    kprintf("\n*** CPU EXCEPTION %lu: %s ***\n", r->int_no, r->int_no < 32 ? names[r->int_no] : "Unknown");
    kprintf("  err=0x%lx  rip=0x%lx  cs=0x%lx  rflags=0x%lx\n", r->err_code, r->rip, r->cs, r->rflags);
    kprintf("  rax=0x%lx rbx=0x%lx rcx=0x%lx rdx=0x%lx\n", r->rax, r->rbx, r->rcx, r->rdx);
    kprintf("  rsi=0x%lx rdi=0x%lx rbp=0x%lx rsp=0x%lx\n", r->rsi, r->rdi, r->rbp, r->rsp);
    if (r->int_no == 14) {
        uint64_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        kprintf("  faulting address (cr2)=0x%lx\n", cr2);
    }
    kprintf("  kernel halted.\n");
    for (;;) __asm__ volatile ("cli; hlt");
}

static void set_gate(int n, void *handler) {
    uint64_t a = (uint64_t)handler;
    idt[n].offset_low = a & 0xFFFF;
    idt[n].selector = 0x08; //kernel code segment
    idt[n].ist = 0;
    idt[n].type_attr = 0x8E; //present, ring 0, 64-bit interrupt gate
    idt[n].offset_mid = (a >> 16) & 0xFFFF;
    idt[n].offset_high = (a >> 32) & 0xFFFFFFFF;
    idt[n].zero = 0;
}

void idt_init(void) {
    for (int i = 0; i < 32; i++) set_gate(i, isr_stub_table[i]);
    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt;
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}