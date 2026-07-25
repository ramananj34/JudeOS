#include <stdint.h> //Get some integers

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

//I/O ports are a seperate address space, so we need outb and inb for I/O ports to talk
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); //Straight assembly
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port)); //Straight assembly AT&T syntax, Line status register
    return ret;
}

//Print a single character
static void serial_putc(char c) {
    while ((inb(0x3fd) & 0x20) == 0) { }
    outb(0x3f8, (uint8_t)c);
}

//Print a array of characters
static void serial_print(const char *s) { while (*s) serial_putc(*s++); }

//Print a number in Base 16
static void serial_hex(uint64_t n) {
    serial_print("0x");
    for (int i = 60; i >= 0; i -= 4) {
        int d = (n >> i) & 0xf;
        serial_putc(d < 10 ? '0' + d : 'a' + d - 10);
    }
}

//Print a number in Base 10
static void serial_dec(uint32_t n) {
    char buf[11]; int i = 10; buf[10] = 0;
    if (n == 0) { serial_putc('0'); return; }
    while (n > 0) { buf[--i] = '0' + (n % 10); n /= 10; }
    serial_print(&buf[i]);
}

//Putting it together
void kmain(boot_info_t *info) {
    //Print boot message
    serial_print("[kernel] up in 64-bit long mode, boot_info received\r\n");

    //Print boot drive
    serial_print("[kernel] boot drive: ");
    serial_hex(info->boot_drive);

    //Print memory map
    serial_print("\r\n[kernel] memory map: ");
    serial_dec(info->mmap_count);
    serial_print(" regions\r\n");
    mmap_entry_t *m = (mmap_entry_t *)info->mmap_addr;
    uint64_t usable = 0;
    for (uint32_t i = 0; i < info->mmap_count; i++) {
        serial_print("  base="); serial_hex(m[i].base);
        serial_print(" len=");   serial_hex(m[i].length);
        serial_print(" type=");  serial_dec(m[i].type);
        serial_print("\r\n");
        if (m[i].type == 1) usable += m[i].length;
    }
    serial_print("[kernel] usable RAM: "); serial_hex(usable);

    //Now we are done
    serial_print(" bytes\r\n[kernel] bootloader handoff complete.\r\n");

    //So we halt
    for (;;) __asm__ volatile ("hlt");
}