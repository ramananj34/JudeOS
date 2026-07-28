#pragma once
#include <stdint.h>
void ata_read(uint32_t lba, uint8_t count, void *buf);
void ata_write(uint32_t lba, uint8_t count, const void *buf);