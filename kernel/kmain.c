#include <stdint.h> //Get some integers
#include "console.h" //Console Line Interface
#include "gdt.h" //Global Descriptor Table
#include "idt.h" //Interrupt Descriptor Table
#include "interrupts.h"
#include "timer.h"
#include "bootinfo.h"
#include "pmm.h" //Physical Memory Manager
#include "vmm.h" //Virtual Memory Manager
#include "kheap.h" //The Heap
#include "thread.h"
#include "spinlock.h"
#include "syscall.h"
#include "process.h"

//Thread tests
/*
static volatile int done = 0;
static void busy(void){ for (volatile uint64_t i = 0; i < 4000000; i++); }
static void tA(void){ for(int i=0;i<12;i++){ kprintf("A"); busy(); } done++; for(;;) busy(); }
static void tB(void){ for(int i=0;i<12;i++){ kprintf("B"); busy(); } done++; for(;;) busy(); }
static void tC(void){ for(int i=0;i<12;i++){ kprintf("C"); busy(); } done++; for(;;) busy(); }
*/

//Spinlock tests
/*
#define ITERS 8
static volatile long counter;
static volatile int fin;
static spinlock_t lk = SPINLOCK_INIT;
//A deliberately WIDE read-modify-write, so a 10ms timer tick reliably lands inside it. This is what exposes the race on a single CPU.
static void inc_wide(void) {
    long t = counter;
    for (volatile int d = 0; d < 1500000; d++);  //widen the window
    counter = t + 1;
}
static void unsafe(void) {
    for (int i = 0; i < ITERS; i++) inc_wide();
    fin++;
    for (;;) __asm__ volatile ("hlt");
}
static void safe(void) {
    for (int i = 0; i < ITERS; i++) { spin_lock(&lk); inc_wide(); spin_unlock(&lk); }
    fin++;
    for (;;) __asm__ volatile ("hlt");
}
*/

//Usermode tests
/*
extern void enter_user(uint64_t entry, uint64_t stack);
//ring-3 machine code: nop; nop; cli (privileged -> #GP); jmp $
static uint8_t user_stub[] = { 0x90, 0x90, 0xFA, 0xEB, 0xFE };
*/

//Syscall tests
/*
extern void enter_user(uint64_t entry, uint64_t stack);
extern uint8_t user_blob[], user_blob_end[];
*/

//Process Tests
/*
extern uint8_t user_blob[];
extern int done_count;
*/

//Putting it together
void kmain(boot_info_t *info) {
    console_init();

    kprintf("[kernel] console up. boot_info at %p\n", info);
    kprintf("[kernel] boot drive: 0x%x\n", info->boot_drive);
    kprintf("[kernel] memory map: %u regions\n", info->mmap_count);
    mmap_entry_t *m = (mmap_entry_t *)info->mmap_addr;
    uint64_t usable = 0;
    for (uint32_t i = 0; i < info->mmap_count; i++) {
        kprintf("  base=0x%lx len=0x%lx type=%u\n", m[i].base, m[i].length, m[i].type);
        if (m[i].type == 1) usable += m[i].length;
    }
    kprintf("[kernel] usable RAM: %lu bytes (0x%lx)\n", usable, usable);

    gdt_init();
    kprintf("[kernel] GDT loaded.\n");
    idt_init();
    kprintf("[kernel] IDT loaded, exception handlers armed.\n");

    pic_remap();
    kprintf("[kernel] PIC remapped to 0x20..0x2F.\n");
    timer_init(100);
    kprintf("[kernel] PIT timer started at 100 Hz.\n");
    __asm__ volatile ("sti");
    kprintf("[kernel] interrupts enabled.\n");
    serial_input_init();
    kprintf("[kernel] Able to recieve serial input.\n");

    /* Time Demo
    uint64_t last = 0;
    for (;;) {
        uint64_t t = timer_ticks();
        if (t / 100 != last) { last = t / 100; kprintf("[kernel] uptime %lu s (%lu ticks)\n", last, t); }
        __asm__ volatile ("hlt");
    } */

    /* Input demo
    for (;;) {
        int c = kgetc();
        if (c >= 0) kprintf("[kernel] got: '%c' (0x%x)\n", (char)c, c);
        __asm__ volatile ("hlt");
    } */

    pmm_init(info);
    kprintf("[pmm] total: %lu frames (%lu MiB)\n", pmm_total_frames(), pmm_total_frames() * 4096 / 1024 / 1024);
    kprintf("[pmm] free:  %lu frames (%lu MiB)\n", pmm_free_frames(), pmm_free_frames() * 4096 / 1024 / 1024);
    void *f = pmm_alloc_frame();
    *(volatile uint64_t *)f = 0xcafebabedeadbeef;
    kprintf("[pmm] test frame %p reads back 0x%lx\n", f, *(volatile uint64_t *)f);
    pmm_free_frame(f);

    vmm_init();
    kprintf("[vmm] own page tables active, NX enabled.\n");
    uint64_t P = (uint64_t)pmm_alloc_frame();
    uint64_t V = 0x40000000; // above the 1 GiB identity map
    vmm_map(V, P, PTE_WRITABLE);
    *(volatile uint64_t *)V = 0x1234567890abcdef;
    kprintf("[vmm] wrote via V=0x%lx, read via P=0x%lx: 0x%lx\n", V, P, *(volatile uint64_t *)P);
    kprintf("[vmm] translate(V)=0x%lx, ", vmm_translate(V));
    vmm_unmap(V);
    kprintf("after unmap=0x%lx\n", vmm_translate(V));

    kprintf("[kernel] kmain runs at %p (higher half)\n", (void *)kmain);
    /* W^X Demo
    kprintf("[test] writing to kmain's code...\n");
    *(volatile uint32_t *)kmain = 0;
    */

    kheap_init();
    /* Heap demo
    void *p = kmalloc(1234);
    kprintf("[heap] kmalloc(1234) = %p\n", p);
    kfree(p);
    */

    sched_init();
    /* Thread demo
    thread_create(tA); thread_create(tB); thread_create(tC);
    kprintf("[sched] 3 non-yielding threads, preempted by the timer: ");
    while (done < 3) busy();
    kprintf("\n[sched] PREEMPTIVE multitasking works.\n");
    */

    /* Spinlock tests
    counter = 0; fin = 0;
    kprintf("[lock] test 1: 3 threads, NO lock, %d increments each...\n", ITERS);
    thread_create(unsafe); thread_create(unsafe); thread_create(unsafe);
    while (fin < 3) __asm__ volatile ("hlt");
    kprintf("[lock] counter = %ld, expected %d -> %s\n", counter, 3 * ITERS, counter == 3 * ITERS ? "OK" : "LOST UPDATES (race!)");
    counter = 0; fin = 0;
    kprintf("[lock] test 2: 3 threads, WITH spinlock, %d increments each...\n", ITERS);
    thread_create(safe); thread_create(safe); thread_create(safe);
    while (fin < 3) __asm__ volatile ("hlt");
    kprintf("[lock] counter = %ld, expected %d -> %s\n", counter, 3 * ITERS, counter == 3 * ITERS ? "CORRECT" : "still wrong");
    */

    /* Ring 3 tests
    uint64_t code = 0x8000000000; //user code page (its own PML4 slot)
    uint64_t stack = 0x8000010000; //user stack page
    vmm_map(code, (uint64_t)pmm_alloc_frame(), PTE_USER | PTE_WRITABLE);
    vmm_map(stack, (uint64_t)pmm_alloc_frame(), PTE_USER | PTE_WRITABLE | PTE_NX);
    for (unsigned i = 0; i < sizeof(user_stub); i++)
        ((volatile uint8_t *)code)[i] = user_stub[i];
    kprintf("[user] dropping to ring 3 at 0x%lx (it will try a privileged 'cli')...\n", code);
    enter_user(code, stack + 4096); //hand ring 3 a stack top
    kprintf("[user] SHOULD NOT REACH HERE\n");
    */

    syscall_init();
    /* Syscall tests
    uint64_t code = 0x8000000000, stack = 0x8000010000;
    vmm_map(code,  (uint64_t)pmm_alloc_frame(), PTE_USER | PTE_WRITABLE);
    vmm_map(stack, (uint64_t)pmm_alloc_frame(), PTE_USER | PTE_WRITABLE | PTE_NX);
    for (uint8_t *p = user_blob; p < user_blob_end; p++)
        ((volatile uint8_t *)code)[p - user_blob] = *p;
    kprintf("[user] entering ring 3; it will make system calls...\n");
    enter_user(code, stack + 4096);
    for (;;) __asm__ volatile ("hlt");
    */

    
    /* Process tests
    __asm__ volatile ("sti");
    kprintf("[proc] two processes, same ELF, isolated address spaces: ");
    process_create(user_blob, 1);
    process_create(user_blob, 2);
    while (done_count < 2) __asm__ volatile ("hlt");
    kprintf("\n[proc] both exited. No '!' means isolation held.\n");
    for (;;) __asm__ volatile ("hlt");
    */

}