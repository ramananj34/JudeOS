#include "syscall.h"
#include "console.h"
#include <stdint.h>

extern void syscall_entry(void); //in usermode.asm
uint64_t syscall_kstack; //kernel stack top for syscall entry
uint64_t saved_user_rsp; //scratch during entry

#define USER_LIMIT 0x0000800000000000ull

//reject any pointer/length that isn't wholly inside user space
static int valid_user_range(uint64_t ptr, uint64_t len) {
    if (ptr == 0) return 0;
    if (ptr >= USER_LIMIT) return 0; //kernel/high addresses forbidden
    if (ptr + len < ptr) return 0; //overflow
    if (ptr + len > USER_LIMIT) return 0;
    return 1;
}

uint64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a3;
    switch (num) {
        case 1: { //SYS_WRITE(ptr, len)
            if (!valid_user_range(a1, a2)) {
                kprintf("[syscall] REJECTED write: bad user pointer 0x%lx\n", a1);
                return (uint64_t)-1;
            }
            const char *s = (const char *)a1;
            for (uint64_t i = 0; i < a2; i++) kputc(s[i]);
            return a2;
        }
        case 0: //SYS_EXIT(code)
            kprintf("[syscall] process exited with code %lu\n", a1);
            for (;;) __asm__ volatile ("cli; hlt");
        default:
            kprintf("[syscall] unknown syscall %lu\n", num);
            return (uint64_t)-1;
    }
}

static void wrmsr(uint32_t msr, uint64_t v) {
    __asm__ volatile ("wrmsr" : : "a"((uint32_t)v), "d"((uint32_t)(v >> 32)), "c"(msr));
}
static uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

void syscall_init(void) {
    static uint8_t sc_stack[16384] __attribute__((aligned(16)));
    syscall_kstack = (uint64_t)sc_stack + sizeof(sc_stack);

    wrmsr(0xC0000080, rdmsr(0xC0000080) | 1); //EFER.SCE = enable syscall
    //STAR: kernel base 0x08 in [47:32], user base 0x10 in [63:48]
    wrmsr(0xC0000081, ((uint64_t)0x08 << 32) | ((uint64_t)0x10 << 48));
    wrmsr(0xC0000082, (uint64_t)syscall_entry); //LSTAR = entry point
    wrmsr(0xC0000084, 0x200 | 0x100 | 0x400); //SFMASK: clear IF, TF, DF on entry
}