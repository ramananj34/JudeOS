//Virtual Memory Manager
//PML4 -> PDPT -> PD -> PT
//Page Table Entry (PTE) flags
//Bootstrap + NX capability

#include "vmm.h"
#include "pmm.h"

#define ENTRIES   512
#define PTE_HUGE  (1ull << 7)
#define ADDR_MASK 0x000FFFFFFFFFF000ull

#define PML4_IDX(v) (((v) >> 39) & 0x1FF)
#define PDPT_IDX(v) (((v) >> 30) & 0x1FF)
#define PD_IDX(v) (((v) >> 21) & 0x1FF)
#define PT_IDX(v) (((v) >> 12) & 0x1FF)

static uint64_t *pml4;

//first 1 GiB is identity-mapped, so a physical frame's virtual address == itself
static inline uint64_t *p2v(uint64_t phys) { return (uint64_t *)phys; }

//walk to the PTE for virt; create intermediate tables if create is set
static uint64_t *walk(uint64_t virt, int create) {
    uint64_t *t = pml4;
    uint64_t idx[3] = { PML4_IDX(virt), PDPT_IDX(virt), PD_IDX(virt) };
    for (int lvl = 0; lvl < 3; lvl++) {
        if (!(t[idx[lvl]] & PTE_PRESENT)) {
            if (!create) return 0;
            uint64_t f = (uint64_t)pmm_alloc_frame();
            uint64_t *nt = p2v(f);
            for (int i = 0; i < ENTRIES; i++) nt[i] = 0;
            t[idx[lvl]] = f | PTE_PRESENT | PTE_WRITABLE;
        }
        t = p2v(t[idx[lvl]] & ADDR_MASK);
    }
    return &t[PT_IDX(virt)];
}

void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t *pte = walk(virt, 1);
    *pte = (phys & ADDR_MASK) | flags | PTE_PRESENT;
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}
void vmm_unmap(uint64_t virt) {
    uint64_t *pte = walk(virt, 0);
    if (pte && (*pte & PTE_PRESENT)) {
        *pte = 0;
        __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
    }
}
uint64_t vmm_translate(uint64_t virt) {
    uint64_t *pte = walk(virt, 0);
    if (!pte || !(*pte & PTE_PRESENT)) return 0;
    return (*pte & ADDR_MASK) | (virt & 0xFFF);
}

void vmm_init(void) {
    //enable the NX capability (EFER.NXE, bit 11) so PTE_NX is honored
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0xC0000080));
    lo |= (1u << 11);
    __asm__ volatile ("wrmsr" : : "a"(lo), "d"(hi), "c"(0xC0000080));

    //fresh PML4 that identity-maps the first 1 GiB with 2 MiB pages
    uint64_t pml4_phys = (uint64_t)pmm_alloc_frame();
    pml4 = p2v(pml4_phys);
    uint64_t pdpt_phys = (uint64_t)pmm_alloc_frame();
    uint64_t pd_phys   = (uint64_t)pmm_alloc_frame();
    uint64_t *pdpt = p2v(pdpt_phys), *pd = p2v(pd_phys);
    for (int i = 0; i < ENTRIES; i++) { pml4[i] = 0; pdpt[i] = 0; pd[i] = 0; }
    pml4[0] = pdpt_phys | PTE_PRESENT | PTE_WRITABLE;
    pdpt[0] = pd_phys | PTE_PRESENT | PTE_WRITABLE;
    for (int i = 0; i < ENTRIES; i++)
        pd[i] = ((uint64_t)i * 0x200000) | PTE_PRESENT | PTE_WRITABLE | PTE_HUGE;

    __asm__ volatile ("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");
}