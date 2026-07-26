#pragma once
#include <stdint.h>
#include "bootinfo.h"

void pmm_init(boot_info_t *info);
void *pmm_alloc_frame(void); // returns a 4KB physical frame, or 0
void pmm_free_frame(void *frame);
uint64_t pmm_free_frames(void);
uint64_t pmm_total_frames(void);