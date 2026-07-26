#include <stdint.h> //Get some integers
#include "console.h" //Console Line Interface
#include "gdt.h" //Global Descriptor Table
#include "idt.h" //Interrupt Descriptor Table
#include "interrupts.h"
#include "timer.h"
#include "bootinfo.h"
#include "pmm.h" //Physical Memory Manager
#include "vmm.h" //Virtual Memory Manager

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
    serial_input_init();
    kprintf("[kernel] Able to recieve serial input.\n");

    /* Time Demo
    uint64_t last = 0;
    for (;;) {
        uint64_t t = timer_ticks();
        if (t / 100 != last) { last = t / 100; kprintf("[kernel] uptime %lu s (%lu ticks)\n", last, t); }
        __asm__ volatile ("hlt");
    } */

    /* Input demo
    for (;;) {
        int c = kgetc();
        if (c >= 0) kprintf("[kernel] got: '%c' (0x%x)\n", (char)c, c);
        __asm__ volatile ("hlt");
    } */

    pmm_init(info);
    kprintf("[pmm] total: %lu frames (%lu MiB)\n", pmm_total_frames(), pmm_total_frames() * 4096 / 1024 / 1024);
    kprintf("[pmm] free:  %lu frames (%lu MiB)\n", pmm_free_frames(), pmm_free_frames() * 4096 / 1024 / 1024);
    void *f = pmm_alloc_frame();
    *(volatile uint64_t *)f = 0xcafebabedeadbeef;
    kprintf("[pmm] test frame %p reads back 0x%lx\n", f, *(volatile uint64_t *)f);
    pmm_free_frame(f);

    vmm_init();
    kprintf("[vmm] own page tables active, NX enabled.\n");
    uint64_t P = (uint64_t)pmm_alloc_frame();
    uint64_t V = 0x40000000; // above the 1 GiB identity map
    vmm_map(V, P, PTE_WRITABLE);
    *(volatile uint64_t *)V = 0x1234567890abcdef;
    kprintf("[vmm] wrote via V=0x%lx, read via P=0x%lx: 0x%lx\n", V, P, *(volatile uint64_t *)P);
    kprintf("[vmm] translate(V)=0x%lx, ", vmm_translate(V));
    vmm_unmap(V);
    kprintf("after unmap=0x%lx\n", vmm_translate(V));

}