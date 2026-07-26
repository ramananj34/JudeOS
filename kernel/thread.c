#include "thread.h"
#include "kheap.h"
#include <stdint.h>

#define MAX_THREADS 16
#define STACK_SIZE  (16 * 1024)

typedef struct { uint64_t rsp; int used; } thread_t;

static thread_t threads[MAX_THREADS];
static int nthreads;
static int current;

extern void context_switch(uint64_t *old_rsp, uint64_t new_rsp);

void sched_init(void) {
    threads[0].used = 1; //thread 0 = the code already running (kmain)
    nthreads = 1;
    current = 0;
}

int thread_create(void (*entry)(void)) {
    if (nthreads >= MAX_THREADS) return -1;
    int i = nthreads++;
    uint64_t stack = (uint64_t)kmalloc(STACK_SIZE);
    uint64_t *sp = (uint64_t *)((stack + STACK_SIZE) & ~0xFull); //16-align the top
    *(--sp) = (uint64_t)entry; //ret target: the thread's entry function
    *(--sp) = 0; //rbx
    *(--sp) = 0; //rbp
    *(--sp) = 0; //r12
    *(--sp) = 0; //r13
    *(--sp) = 0; //r14
    *(--sp) = 0; //r15
    threads[i].rsp  = (uint64_t)sp;
    threads[i].used = 1;
    return i;
}

void yield(void) {
    if (nthreads < 2) return;
    int prev = current;
    current = (current + 1) % nthreads; //round-robin
    context_switch(&threads[prev].rsp, threads[current].rsp);
}