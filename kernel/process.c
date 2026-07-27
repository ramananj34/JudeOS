#include "process.h"
#include "vmm.h"
#include "pmm.h"
#include "kheap.h"
#include "thread.h"

#define USER_STACK_TOP 0x8001000000ull

static uint64_t load_user_elf(uint64_t cr3, uint8_t *b) {
    uint64_t phoff = *(uint64_t *)(b + 0x20);
    uint16_t phnum = *(uint16_t *)(b + 0x38);
    uint16_t phent = *(uint16_t *)(b + 0x36);
    for (int i = 0; i < phnum; i++) {
        uint8_t *ph = b + phoff + (uint64_t)i * phent;
        if (*(uint32_t *)(ph + 0) != 1) continue; //PT_LOAD
        uint64_t off = *(uint64_t *)(ph + 0x08);
        uint64_t va = *(uint64_t *)(ph + 0x10) & ~0xFFFull;
        uint64_t fsz = *(uint64_t *)(ph + 0x20);
        uint64_t msz = *(uint64_t *)(ph + 0x28);
        uint32_t fl = *(uint32_t *)(ph + 0x04);
        uint64_t pf = PTE_USER;
        if (fl & 2) pf |= PTE_WRITABLE;
        if (!(fl & 1)) pf |= PTE_NX;
        for (uint64_t p = 0; p < msz; p += 4096) {
            uint64_t frame = (uint64_t)pmm_alloc_frame();
            uint8_t *dst = (uint8_t *)frame;               // direct-map access
            for (int k = 0; k < 4096; k++) dst[k] = 0;
            if (p < fsz) { uint64_t n = fsz - p; if (n > 4096) n = 4096; for (uint64_t k = 0; k < n; k++) dst[k] = b[off + p + k]; }
            vmm_map_to(cr3, va + p, frame, pf);
        }
    }
    return *(uint64_t *)(b + 0x18); //e_entry
}

int process_create(uint8_t *blob, int pid) {
    uint64_t cr3 = vmm_new_addrspace();
    uint64_t entry = load_user_elf(cr3, blob);
    for (int i = 1; i <= 4; i++)
        vmm_map_to(cr3, USER_STACK_TOP - 0x1000 * i, (uint64_t)pmm_alloc_frame(), PTE_USER | PTE_WRITABLE | PTE_NX);
    uint64_t kstack_top = ((uint64_t)kmalloc(16384) + 16384) & ~0xFull;
    return thread_create_user(entry, USER_STACK_TOP, cr3, kstack_top, pid);
}