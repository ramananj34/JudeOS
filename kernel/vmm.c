//Virtual Memory Manager
//PML4 -> PDPT -> PD -> PT
//Page Table Entry (PTE) flags
//Bootstrap + NX capability + C^X

#include "vmm.h"
#include "pmm.h"

#define ENTRIES 512
#define PTE_HUGE (1ull << 7)
#define ADDR_MASK 0x000FFFFFFFFFF000ull
#define KOFF 0xffffffff80000000ull

#define PML4_IDX(v) (((v) >> 39) & 0x1FF)
#define PDPT_IDX(v) (((v) >> 30) & 0x1FF)
#define PD_IDX(v) (((v) >> 21) & 0x1FF)
#define PT_IDX(v) (((v) >> 12) & 0x1FF)

static uint64_t *pml4;
static inline uint64_t *p2v(uint64_t phys) { return (uint64_t *)phys; }

static uint64_t *walk(uint64_t virt, int create) {
    uint64_t *t = pml4;
    uint64_t idx[3] = { PML4_IDX(virt), PDPT_IDX(virt), PD_IDX(virt) };
    for (int lvl = 0; lvl < 3; lvl++) {
        if (!(t[idx[lvl]] & PTE_PRESENT)) {
            if (!create) return 0;
            uint64_t f = (uint64_t)pmm_alloc_frame();
            uint64_t *nt = p2v(f);
            for (int i = 0; i < ENTRIES; i++) nt[i] = 0;
            t[idx[lvl]] = f | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
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
    if (pte && (*pte & PTE_PRESENT)) { *pte = 0; __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory"); }
}

uint64_t vmm_translate(uint64_t virt) {
    uint64_t *pte = walk(virt, 0);
    if (!pte || !(*pte & PTE_PRESENT)) return 0;
    return (*pte & ADDR_MASK) | (virt & 0xFFF);
}

//kernel section boundaries (from linker.ld)
extern char _text_start[], _text_end[];
extern char _rodata_start[], _rodata_end[];
extern char _data_start[], _data_end[];
extern char _bss_start[], _bss_end[];

static void map_section(uint64_t vs, uint64_t ve, uint64_t flags) {
    vs &= ~0xFFFull;
    ve = (ve + 0xFFF) & ~0xFFFull;
    for (uint64_t v = vs; v < ve; v += 4096)
        vmm_map(v, v - KOFF, flags); //phys = virt - higher-half offset
}

void vmm_init(void) {
    //enable NX (EFER.NXE)
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0xC0000080));
    lo |= (1u << 11);
    __asm__ volatile ("wrmsr" : : "a"(lo), "d"(hi), "c"(0xC0000080));

    //enable CR0.WP so the kernel itself must obey read-only pages
    uint64_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1ull << 16);
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));

    uint64_t pml4_phys = (uint64_t)pmm_alloc_frame();
    pml4 = p2v(pml4_phys);
    for (int i = 0; i < ENTRIES; i++) pml4[i] = 0;

    //low direct map: first 1 GiB with 2 MiB pages, writable but NON-executable
    uint64_t pdpt_phys = (uint64_t)pmm_alloc_frame();
    uint64_t pd_phys   = (uint64_t)pmm_alloc_frame();
    uint64_t *pdpt = p2v(pdpt_phys), *pd = p2v(pd_phys);
    for (int i = 0; i < ENTRIES; i++) { pdpt[i] = 0; pd[i] = 0; }
    pml4[0] = pdpt_phys | PTE_PRESENT | PTE_WRITABLE;
    pdpt[0] = pd_phys   | PTE_PRESENT | PTE_WRITABLE;
    for (int i = 0; i < ENTRIES; i++)
        pd[i] = ((uint64_t)i * 0x200000) | PTE_PRESENT | PTE_WRITABLE | PTE_HUGE | PTE_NX;

    //higher-half kernel: 4 KiB pages with real per-section permissions
    map_section((uint64_t)_text_start, (uint64_t)_text_end, PTE_PRESENT); //exec, read-only
    map_section((uint64_t)_rodata_start, (uint64_t)_rodata_end, PTE_PRESENT | PTE_NX); //read-only, no exec
    map_section((uint64_t)_data_start, (uint64_t)_data_end, PTE_PRESENT | PTE_WRITABLE | PTE_NX); //read/write, no exec
    map_section((uint64_t)_bss_start, (uint64_t)_bss_end, PTE_PRESENT | PTE_WRITABLE | PTE_NX); //read/write, no exec

    __asm__ volatile ("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");
}