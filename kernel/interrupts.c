//8259 PIC not an APIC
//IDT is for exceptions. Hardware just interupts
//IRQ master and slave (16 total)
//Masking and EOI (end of interupt)

#include "interrupts.h"

static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void io_wait(void){ outb(0x80, 0); }

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

static irq_handler_t handlers[16];

void pic_remap(void) {
    outb(PIC1, 0x11); io_wait(); // start init sequence
    outb(PIC2, 0x11); io_wait();
    outb(PIC1_DATA, 0x20); io_wait(); // master IRQs -> vectors 0x20..0x27
    outb(PIC2_DATA, 0x28); io_wait(); // slave  IRQs -> vectors 0x28..0x2F
    outb(PIC1_DATA, 0x04); io_wait(); // master: slave is on IRQ2
    outb(PIC2_DATA, 0x02); io_wait(); // slave: cascade identity
    outb(PIC1_DATA, 0x01); io_wait(); // 8086 mode
    outb(PIC2_DATA, 0x01); io_wait();
    outb(PIC1_DATA, 0xFF); // mask everything for now
    outb(PIC2_DATA, 0xFF);
}

void pic_unmask(int irq) {
    uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    outb(port, inb(port) & ~(1 << irq));
}
void pic_mask(int irq) {
    uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    outb(port, inb(port) | (1 << irq));
}
static void pic_eoi(int irq) {
    if (irq >= 8) outb(PIC2, 0x20); // end-of-interrupt to slave
    outb(PIC1, 0x20); //and always to master
}

void irq_install_handler(int irq, irq_handler_t h) { handlers[irq] = h; }

static uint16_t pic_get_isr(void) {
    outb(PIC1, 0x0B); outb(PIC2, 0x0B); // OCW3: read the In-Service Register
    return ((uint16_t)inb(PIC2) << 8) | inb(PIC1);
}

//called from irq_common (isr.asm) with the IRQ number in rdi
void irq_dispatch(uint64_t irq) {
    if (irq == 7  && !(pic_get_isr() & (1 << 7)))  return; // spurious: no EOI
    if (irq == 15 && !(pic_get_isr() & (1 << 15))) { outb(PIC1, 0x20); return; } // spurious: EOI master only
    if (irq < 16 && handlers[irq]) handlers[irq]();
    pic_eoi((int)irq);
}