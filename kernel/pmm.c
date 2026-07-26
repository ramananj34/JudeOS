//Physical memory manager. Bitmap tracking of E820 map

#include "pmm.h"
#include "console.h"

#define PAGE 4096

extern char _kernel_end[]; // end of the kernel image (from linker.ld)

static uint8_t  *bitmap; // 1 bit per frame: 1 = used, 0 = free
static uint64_t  total_frames;
static uint64_t  used_frames;
static uint64_t  hint; // where to start the next search

static inline void bm_set (uint64_t f){ bitmap[f/8] |=  (1u << (f & 7)); }
static inline void bm_clr (uint64_t f){ bitmap[f/8] &= ~(1u << (f & 7)); }
static inline int  bm_test(uint64_t f){ return bitmap[f/8] & (1u << (f & 7)); }

static void mark_used(uint64_t base, uint64_t len) {
    uint64_t first = base / PAGE; // round outward
    uint64_t last  = (base + len + PAGE - 1) / PAGE;
    for (uint64_t f = first; f < last && f < total_frames; f++)
        if (!bm_test(f)) { bm_set(f); used_frames++; }
}
static void mark_free(uint64_t base, uint64_t len) {
    uint64_t first = (base + PAGE - 1) / PAGE; // round inward
    uint64_t last  = (base + len) / PAGE;
    for (uint64_t f = first; f < last && f < total_frames; f++)
        if (bm_test(f)) { bm_clr(f); used_frames--; }
}

void pmm_init(boot_info_t *info) {
    mmap_entry_t *m = (mmap_entry_t *)info->mmap_addr;

    uint64_t max_addr = 0;
    for (uint32_t i = 0; i < info->mmap_count; i++)
        if (m[i].type == 1 && m[i].base + m[i].length > max_addr)
            max_addr = m[i].base + m[i].length;
    total_frames = max_addr / PAGE;

    uint64_t kend_phys = (uint64_t)_kernel_end - 0xffffffff80000000ull; // high virt -> physical
    bitmap = (uint8_t *)((kend_phys + PAGE - 1) & ~(uint64_t)(PAGE - 1));
    uint64_t bitmap_bytes = (total_frames + 7) / 8;

    for (uint64_t i = 0; i < bitmap_bytes; i++) bitmap[i] = 0xFF; // all used
    used_frames = total_frames;

    for (uint32_t i = 0; i < info->mmap_count; i++) // free usable RAM
        if (m[i].type == 1) mark_free(m[i].base, m[i].length);

    mark_used(0, 0x100000); // reserve low 1 MiB
    uint64_t bend = (uint64_t)bitmap + bitmap_bytes; // reserve kernel + bitmap
    mark_used(0x100000, bend - 0x100000);

    hint = 0;
}

void *pmm_alloc_frame(void) {
    for (uint64_t i = 0; i < total_frames; i++) {
        uint64_t f = (hint + i) % total_frames;
        if (!bm_test(f)) {
            bm_set(f); used_frames++; hint = f + 1;
            return (void *)(f * PAGE);
        }
    }
    return 0; // out of memory
}
void pmm_free_frame(void *frame) {
    uint64_t f = (uint64_t)frame / PAGE;
    if (f < total_frames && bm_test(f)) { bm_clr(f); used_frames--; }
}
uint64_t pmm_free_frames(void)  { return total_frames - used_frames; }
uint64_t pmm_total_frames(void) { return total_frames; }