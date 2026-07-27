#include "thread.h"
#include "kheap.h"
#include "interrupts.h"
#include "vmm.h"
#include "gdt.h"
#include <stdint.h>

#define MAX_THREADS 16

typedef struct {
    uint64_t rsp, cr3, kstack_top;
    int pid, is_user, used, done;
} thread_t;

static thread_t threads[MAX_THREADS];
static int nthreads, current;

int current_pid;
int done_count;
extern uint64_t syscall_kstack;

extern void context_switch(uint64_t *old_rsp, uint64_t new_rsp);
extern void thread_trampoline(void);
extern void process_user_trampoline(void);

static inline void write_cr3(uint64_t v){ __asm__ volatile("mov %0,%%cr3"::"r"(v):"memory"); }

void schedule(void) {
    int start = current, next = current;
    do { next = (next + 1) % nthreads; } while (threads[next].done && next != start);
    if (next == current) return;
    thread_t *n = &threads[next];
    write_cr3(n->cr3);
    if (n->is_user) { tss_set_rsp0(n->kstack_top); syscall_kstack = n->kstack_top; current_pid = n->pid; }
    int prev = current; current = next;
    context_switch(&threads[prev].rsp, n->rsp);
}

void sched_init(void) {
    threads[0].used = 1;
    threads[0].cr3  = vmm_kernel_cr3();
    nthreads = 1; current = 0;
    irq_set_tick_hook(schedule);
}

int thread_create_user(uint64_t entry, uint64_t user_stack, uint64_t cr3, uint64_t kstack_top, int pid) {
    if (nthreads >= MAX_THREADS) return -1;
    int i = nthreads++;
    uint64_t *sp = (uint64_t *)(kstack_top & ~0xFull);
    *(--sp) = (uint64_t)process_user_trampoline;
    *(--sp) = 0; *(--sp) = 0; *(--sp) = 0; *(--sp) = 0; //rbx rbp r12 r13
    *(--sp) = user_stack; //r14
    *(--sp) = entry; //r15
    threads[i].rsp = (uint64_t)sp;
    threads[i].cr3 = cr3;
    threads[i].kstack_top = kstack_top;
    threads[i].pid = pid;
    threads[i].is_user = 1;
    threads[i].used = 1;
    return i;
}

void thread_exit(void) { //called from SYS_EXIT (never returns)
    threads[current].done = 1;
    done_count++;
    schedule();
    for (;;) __asm__ volatile ("hlt"); //unreachable
}