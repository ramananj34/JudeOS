// Programmable Interval Timer (PIT)

#include "timer.h"
#include "interrupts.h"

static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }

static volatile uint64_t ticks;
static void timer_handler(void) { ticks++; }
uint64_t timer_ticks(void) { return ticks; }

void timer_init(uint32_t hz) {
    uint32_t divisor = 1193182 / hz; // PIT base frequency / desired Hz
    outb(0x43, 0x36); // channel 0, lo+hi byte, mode 3
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    irq_install_handler(0, timer_handler);
    pic_unmask(0); // enable IRQ0 (the timer)
}