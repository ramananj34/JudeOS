#include <stdint.h> //Get some integers
#include "console.h" //CLI
#include "gdt.h" //GDT
#include "idt.h" //IDT
#include "interrupts.h"
#include "timer.h"

//Memory map information
typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type; //1=usable 2=reserved 3=ACPI-reclaim 4=ACPI-NVS 5=bad
    uint32_t acpi;
} __attribute__((packed)) mmap_entry_t; //Stop padding

//Boot information
typedef struct {
    uint32_t mmap_count;
    uint32_t boot_drive;
    uint64_t mmap_addr;
    uint64_t acpi_rsdp;
} __attribute__((packed)) boot_info_t; //Stop padding

//Putting it together
void kmain(boot_info_t *info) {
    console_init();

    kprintf("[kernel] console up. boot_info at %p\n", info);
    kprintf("[kernel] boot drive: 0x%x\n", info->boot_drive);
    kprintf("[kernel] memory map: %u regions\n", info->mmap_count);
    mmap_entry_t *m = (mmap_entry_t *)info->mmap_addr;
    uint64_t usable = 0;
    for (uint32_t i = 0; i < info->mmap_count; i++) {
        kprintf("  base=0x%lx len=0x%lx type=%u\n", m[i].base, m[i].length, m[i].type);
        if (m[i].type == 1) usable += m[i].length;
    }
    kprintf("[kernel] usable RAM: %lu bytes (0x%lx)\n", usable, usable);

    gdt_init();
    kprintf("[kernel] GDT loaded.\n");
    idt_init();
    kprintf("[kernel] IDT loaded, exception handlers armed.\n");

    pic_remap();
    kprintf("[kernel] PIC remapped to 0x20..0x2F.\n");
    timer_init(100);
    kprintf("[kernel] PIT timer started at 100 Hz.\n");
    __asm__ volatile ("sti");
    kprintf("[kernel] interrupts enabled.\n");

    uint64_t last = 0;
    for (;;) {
        uint64_t t = timer_ticks();
        if (t / 100 != last) { last = t / 100; kprintf("[kernel] uptime %lu s (%lu ticks)\n", last, t); }
        __asm__ volatile ("hlt");
    }
}