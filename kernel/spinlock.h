#pragma once
typedef struct { volatile int locked; } spinlock_t;
#define SPINLOCK_INIT { 0 }
 
void spin_lock(spinlock_t *l);
void spin_unlock(spinlock_t *l);
unsigned long spin_lock_irqsave(spinlock_t *l);
void spin_unlock_irqrestore(spinlock_t *l, unsigned long flags);