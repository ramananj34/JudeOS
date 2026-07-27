//Three-entry descriptor table (null, kernel code, kernel data)

#include "gdt.h"
#include <stdint.h>

static uint64_t gdt[7];
static struct __attribute__((packed)) { uint16_t limit; uint64_t base; } gdtr;

struct __attribute__((packed)) tss {
    uint32_t reserved0;
    uint64_t rsp0, rsp1, rsp2;
    uint64_t reserved1;
    uint64_t ist1, ist2, ist3, ist4, ist5, ist6, ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
};
static struct tss tss;
static uint8_t kstack[16384] __attribute__((aligned(16)));

void gdt_init(void) {
    gdt[0] = 0;
    gdt[1] = 0x00AF9A000000FFFF; //0x08 kernel code (ring 0)
    gdt[2] = 0x00CF92000000FFFF; //0x10 kernel data (ring 0)
    gdt[3] = 0x00CFF2000000FFFF; //0x18 user data (ring 3)
    gdt[4] = 0x00AFFA000000FFFF; //0x20 user code (ring 3)

    tss.rsp0 = (uint64_t)kstack + sizeof(kstack); //stack used on ring3->ring0
    tss.iomap_base = sizeof(struct tss); //no I/O bitmap: ring 3 gets no port I/O
    uint64_t base = (uint64_t)&tss;
    uint32_t limit = sizeof(struct tss) - 1;
    uint64_t d = (limit & 0xFFFF) | ((base & 0xFFFFFF) << 16) | ((uint64_t)0x89 << 40) /*present, 64-bit available TSS */ | ((uint64_t)((limit >> 16) & 0xF) << 48) | (((base >> 24) & 0xFF) << 56);
    gdt[5] = d; //0x28 TSS (low)
    gdt[6] = (base >> 32) & 0xFFFFFFFF; //TSS (high)

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)&gdt;
    __asm__ volatile ("lgdt %0" : : "m"(gdtr));
    __asm__ volatile ("mov $0x10, %%ax\n mov %%ax, %%ds\n mov %%ax, %%es\n mov %%ax, %%ss\n" "mov %%ax, %%fs\n mov %%ax, %%gs\n" "lea 1f(%%rip), %%rax\n push $0x08\n push %%rax\n lretq\n 1:\n" : : : "rax", "memory");
    uint16_t tr = 0x28;
    __asm__ volatile ("ltr %0" : : "r"(tr)); //load the task register
}