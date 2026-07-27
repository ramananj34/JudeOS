#pragma once
#include <stdint.h>
void sched_init(void);
void schedule(void);
int  thread_create_user(uint64_t entry, uint64_t user_stack, uint64_t cr3, uint64_t kstack_top, int pid);
void thread_exit(void);