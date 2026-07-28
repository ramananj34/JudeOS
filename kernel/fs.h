#pragma once
#include <stdint.h>
void fs_init(void);
int fs_open(const char *name); //-> file index, or -1
uint32_t fs_size(int idx);
int fs_read(int idx, uint32_t off, void *buf, uint32_t len);