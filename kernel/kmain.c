#include <stdint.h> //Get some integers
#include "console.h" //CLI

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
    for (;;) __asm__ volatile ("hlt");
}