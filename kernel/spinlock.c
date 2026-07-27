#include "spinlock.h"

//atomically swap *p with v, returning the OLD value (the core of a spinlock)
static inline int xchg(volatile int *p, int v) {
    __asm__ volatile ("lock xchg %0, %1" : "+r"(v), "+m"(*p) : : "memory");
    return v;
}

void spin_lock(spinlock_t *l) {
    while (xchg(&l->locked, 1) != 0) //if old value was already 1, keep spinning
        __asm__ volatile ("pause");
}
void spin_unlock(spinlock_t *l) {
    __asm__ volatile ("" : : : "memory"); //don't let the compiler move writes past here
    l->locked = 0;
}

//interrupt-safe: disable interrupts while the lock is held (prevents IRQ-handler deadlock)
unsigned long spin_lock_irqsave(spinlock_t *l) {
    unsigned long flags;
    __asm__ volatile ("pushfq; pop %0; cli" : "=r"(flags) : : "memory");
    spin_lock(l);
    return flags;
}

void spin_unlock_irqrestore(spinlock_t *l, unsigned long flags) {
    spin_unlock(l);
    __asm__ volatile ("push %0; popfq" : : "r"(flags) : "memory", "cc");
}