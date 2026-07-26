#pragma once
void sched_init(void);
int  thread_create(void (*entry)(void));
void schedule(void);