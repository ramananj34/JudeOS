//Heap with allocation+splitting, freeing+coalese, growth


#include "kheap.h"
#include "pmm.h"
#include "vmm.h"
#include <stdint.h>

#define HEAP_BASE 0xffffffffc0000000ull //dedicated heap region, above the kernel
#define PAGE 4096
#define ALIGN16(x) (((x) + 15) & ~(size_t)15)

typedef struct { uint64_t size; uint64_t free; } header_t; // 16-byte block header

static uint64_t heap_start;
static uint64_t heap_end; //current mapped extent (exclusive)

static void map_pages(uint64_t from, uint64_t to) {
    for (uint64_t v = from; v < to; v += PAGE)
        vmm_map(v, (uint64_t)pmm_alloc_frame(), PTE_PRESENT | PTE_WRITABLE | PTE_NX);
}

void kheap_init(void) {
    heap_start = HEAP_BASE;
    uint64_t initial = 16 * PAGE; //64 KiB to start
    map_pages(HEAP_BASE, HEAP_BASE + initial);
    heap_end = HEAP_BASE + initial;
    header_t *h = (header_t *)heap_start;
    h->size = initial - sizeof(header_t);
    h->free = 1;
}

static void coalesce(void) {
    uint64_t p = heap_start;
    while (p < heap_end) {
        header_t *h = (header_t *)p;
        uint64_t next = p + sizeof(header_t) + h->size;
        if (h->free && next < heap_end && ((header_t *)next)->free) {
            h->size += sizeof(header_t) + ((header_t *)next)->size; //merge neighbour
            continue; //and re-check
        }
        p = next;
    }
}

static void grow(size_t need) {
    uint64_t add = (need + sizeof(header_t) + PAGE - 1) & ~(uint64_t)(PAGE - 1);
    if (add < 16 * PAGE) add = 16 * PAGE;
    uint64_t old_end = heap_end;
    map_pages(heap_end, heap_end + add);
    heap_end += add;
    header_t *h = (header_t *)old_end;
    h->size = add - sizeof(header_t);
    h->free = 1;
    coalesce();
}

void *kmalloc(size_t size) {
    if (size == 0) return 0;
    size = ALIGN16(size);
    for (int attempt = 0; attempt < 2; attempt++) {
        uint64_t p = heap_start;
        while (p < heap_end) {
            header_t *h = (header_t *)p;
            if (h->free && h->size >= size) {
                if (h->size >= size + sizeof(header_t) + 16) { //split
                    header_t *nh = (header_t *)(p + sizeof(header_t) + size);
                    nh->size = h->size - size - sizeof(header_t);
                    nh->free = 1;
                    h->size = size;
                }
                h->free = 0;
                return (void *)(p + sizeof(header_t));
            }
            p += sizeof(header_t) + h->size;
        }
        grow(size); //out of room: grow, retry
    }
    return 0;
}

void kfree(void *ptr) {
    if (!ptr) return;
    ((header_t *)((uint64_t)ptr - sizeof(header_t)))->free = 1;
    coalesce();
}