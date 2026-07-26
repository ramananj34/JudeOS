#include "console.h"
#include <stdint.h>
#include <stdarg.h>
#include "interrupts.h"

#define COM1 0x3f8

//I/O Writing straight assembly
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

//I/O Writing straight assembly
static inline uint8_t inb(uint16_t port) {
    uint8_t r;
    __asm__ volatile ("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

void console_init(void) {
    outb(COM1 + 1, 0x00); //disable UART interrupts
    outb(COM1 + 3, 0x80); //enable DLAB to set the baud divisor
    outb(COM1 + 0, 0x01); //divisor low  = 1  -> 115200 baud
    outb(COM1 + 1, 0x00); //divisor high = 0
    outb(COM1 + 3, 0x03); //8 bits, no parity, 1 stop; clear DLAB
    outb(COM1 + 2, 0xC7); //enable + clear FIFOs
    outb(COM1 + 4, 0x0B); //RTS/DSR set, OUT2 on (needed for IRQs later)
}

//Print a character
void kputc(char c) {
    if (c == '\n') { // translate LF -> CRLF
        while ((inb(COM1 + 5) & 0x20) == 0) { }
        outb(COM1, '\r');
    }
    while ((inb(COM1 + 5) & 0x20) == 0) { } // wait: transmit reg empty
    outb(COM1, (uint8_t)c);
}

//Print a string
void kputs(const char *s) { while (*s) kputc(*s++); }

//print an unsigned value in base 2..16, zero/space padded to `width`
static void kput_uint(uint64_t v, int base, int width, char pad) {
    static const char d[] = "0123456789abcdef";
    char buf[65];
    int i = 0;
    if (v == 0) buf[i++] = '0';
    while (v) { buf[i++] = d[v % base]; v /= base; }
    while (i < width) buf[i++] = pad;
    while (i) kputc(buf[--i]);
}

//Print an integer more simply
static void kput_int(int64_t v) {
    if (v < 0) { kputc('-'); kput_uint((uint64_t)(-v), 10, 0, ' '); }
    else kput_uint((uint64_t)v, 10, 0, ' ');
}

//Print a f string
void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    for (; *fmt; fmt++) {
        if (*fmt != '%') { kputc(*fmt); continue; }
        fmt++;
        switch (*fmt) {
            case 'c': kputc((char)va_arg(ap, int)); break;
            case 's': kputs(va_arg(ap, const char *)); break;
            case 'd': kput_int(va_arg(ap, int)); break;
            case 'u': kput_uint(va_arg(ap, unsigned int), 10, 0, ' '); break;
            case 'x': kput_uint(va_arg(ap, unsigned int), 16, 0, ' '); break;
            case 'p': kputs("0x"); kput_uint(va_arg(ap, uint64_t), 16, 16, '0'); break;
            case 'l': // 64-bit: %ld %lu %lx
                fmt++;
                if (*fmt == 'x') kput_uint(va_arg(ap, uint64_t), 16, 0, ' ');
                else if (*fmt == 'u') kput_uint(va_arg(ap, uint64_t), 10, 0, ' ');
                else if (*fmt == 'd') { int64_t v = va_arg(ap, int64_t); if (v < 0) { kputc('-'); kput_uint((uint64_t)(-v),10,0,' '); } else kput_uint((uint64_t)v,10,0,' '); }
                break;
            case '%': kputc('%'); break;
            default:  kputc('%'); kputc(*fmt); break;
        }
    }
    va_end(ap);
}

//Do some UART
//serial input (interrupt-driven)
#define IN_BUF 256
static volatile char inbuf[IN_BUF];
static volatile uint32_t in_head, in_tail;

static void serial_irq(void) {
    while (inb(COM1 + 5) & 0x01) { // while receive-data-ready
        char c = (char)inb(COM1);
        uint32_t next = (in_head + 1) % IN_BUF;
        if (next != in_tail) { inbuf[in_head] = c; in_head = next; } // else drop
    }
}

void serial_input_init(void) {
    outb(COM1 + 1, 0x01); // IER: enable RX interrupt
    irq_install_handler(4, serial_irq); // COM1 -> IRQ4
    pic_unmask(4);
}

int kgetc(void) { // non-blocking; -1 if empty
    if (in_tail == in_head) return -1;
    char c = inbuf[in_tail];
    in_tail = (in_tail + 1) % IN_BUF;
    return (int)(unsigned char)c;
}