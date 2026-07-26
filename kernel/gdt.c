//Three-entry descriptor table (null, kernel code, kernel data)

#include "gdt.h"
#include <stdint.h>

static uint64_t gdt[3];

static struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} gdtr;

void gdt_init(void) {
    gdt[0] = 0; // null descriptor
    gdt[1] = 0x00AF9A000000FFFF; // kernel code: 64-bit, ring 0, exec/read
    gdt[2] = 0x00CF92000000FFFF; // kernel data: ring 0, read/write

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)&gdt;
    __asm__ volatile ("lgdt %0" : : "m"(gdtr));

    //reload data segment registers, then reload CS via a far return
    __asm__ volatile (
        "mov $0x10, %%ax \n"
        "mov %%ax, %%ds \n"
        "mov %%ax, %%es \n"
        "mov %%ax, %%ss \n"
        "mov %%ax, %%fs \n"
        "mov %%ax, %%gs \n"
        "lea 1f(%%rip), %%rax\n"
        "push $0x08 \n"
        "push %%rax \n"
        "lretq \n"
        "1: \n"
        : : : "rax", "memory"
    );
}