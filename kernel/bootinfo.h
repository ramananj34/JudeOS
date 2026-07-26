#pragma once
#include <stdint.h>

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type; /* 1=usable 2=reserved 3=ACPI-reclaim 4=ACPI-NVS 5=bad */
    uint32_t acpi;
} __attribute__((packed)) mmap_entry_t;

typedef struct {
    uint32_t mmap_count;
    uint32_t boot_drive;
    uint64_t mmap_addr;
    uint64_t acpi_rsdp;
} __attribute__((packed)) boot_info_t;