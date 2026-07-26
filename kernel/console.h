#pragma once
#include <stdint.h>
 
void console_init(void);
void kputc(char c);
void kputs(const char *s);
void kprintf(const char *fmt, ...);
void serial_input_init(void);
int kgetc(void);