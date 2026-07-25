#pragma once
#include <stdint.h>
 
void console_init(void);
void kputc(char c);
void kputs(const char *s);
void kprintf(const char *fmt, ...);
 