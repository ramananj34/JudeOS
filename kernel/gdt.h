#pragma once
#include <stdint.h>
void gdt_init(void);
void tss_set_rsp0(uint64_t rsp0);