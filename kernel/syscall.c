#include "syscall.h"
#include "thread.h"
extern int current_pid;
#include "console.h"
#include "fs.h"
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

#define MAXPID 16
#define MAXFD  16
static struct { int used, fileidx; uint32_t off; } fdt[MAXPID][MAXFD];

static void copy_user_str(uint64_t uptr, char *dst, int max) {
    const char *s = (const char *)uptr;
    int i = 0; for (; i < max-1 && s[i]; i++) dst[i] = s[i]; dst[i] = 0;
}

uint64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a3;
    switch (num) {
        case 1: { //SYS_WRITE(fd, ptr, len)
            if (!valid_user_range(a2, a3)) return (uint64_t)-1;
            const char *s = (const char *)a2;
            for (uint64_t i = 0; i < a3; i++) kputc(s[i]);
            return a3;
        }
        case 3: { //SYS_READ(fd, ptr, len)
            if (!valid_user_range(a2, a3)) return (uint64_t)-1;
            if (a1 == 0) { //stdin: serial (non-blocking)
                char *d = (char *)a2; uint64_t n = 0;
                while (n < a3) { int c = kgetc(); if (c < 0) break; d[n++] = (char)c; }
                return n;
            }
            if (a1 >= 3 && a1 < MAXFD && fdt[current_pid][a1].used) { //file
                int idx = fdt[current_pid][a1].fileidx;
                uint32_t off = fdt[current_pid][a1].off;
                int r = fs_read(idx, off, (void *)a2, a3);
                if (r > 0) fdt[current_pid][a1].off += r;
                return (uint64_t)(int64_t)r;
            }
            return 0;
        }
        case 4: { //SYS_OPEN(path)
            if (!valid_user_range(a1, 1)) return (uint64_t)-1;
            char path[64]; copy_user_str(a1, path, sizeof path);
            int idx = fs_open(path);
            if (idx < 0) return (uint64_t)-1;
            for (int fd = 3; fd < MAXFD; fd++)
                if (!fdt[current_pid][fd].used) {
                    fdt[current_pid][fd].used = 1;
                    fdt[current_pid][fd].fileidx = idx;
                    fdt[current_pid][fd].off = 0;
                    return fd;
                }
            return (uint64_t)-1;
        }
        case 5: //SYS_CLOSE(fd)
            if (a1 < MAXFD) fdt[current_pid][a1].used = 0;
            return 0;
        case 2: //SYS_GETPID
            return (uint64_t)current_pid;
        case 0: //SYS_EXIT
            kprintf("\n[syscall] pid %d exited\n", current_pid);
            thread_exit();
            return 0;
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
    wrmsr(0xC0000082, (uint64_t)syscall_entry); // LSTAR = entry point
    wrmsr(0xC0000084, 0x200 | 0x100 | 0x400); //SFMASK: clear IF, TF, DF on entry
}