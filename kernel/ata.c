#include "ata.h"

static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb (uint16_t p){ uint8_t  r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline uint16_t inw (uint16_t p){ uint16_t r; __asm__ volatile("inw %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void outw(uint16_t p, uint16_t v){ __asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p)); }

#define DATA 0x1F0
#define SECCNT 0x1F2
#define LBA0 0x1F3
#define LBA1 0x1F4
#define LBA2 0x1F5
#define DRIVE 0x1F6
#define CMD 0x1F7   //write = command, read = status

static void wait_ready(void){ for (int i=0;i<1000000;i++) if(!(inb(CMD)&0x80)) return; } //BSY clear
static int wait_drq(void){ for (int i=0;i<1000000;i++){ uint8_t s=inb(CMD); if(s&0x01) return -1; if(s&0x08) return 0; } return -1; }

static void setup(uint32_t lba, uint8_t count){
    wait_ready();
    outb(DRIVE, 0xE0 | ((lba >> 24) & 0x0F)); //master, LBA mode, high nibble
    outb(SECCNT, count);
    outb(LBA0, lba & 0xFF);
    outb(LBA1, (lba >> 8) & 0xFF);
    outb(LBA2, (lba >> 16) & 0xFF);
}

void ata_read(uint32_t lba, uint8_t count, void *buf){
    setup(lba, count);
    outb(CMD, 0x20); //READ SECTORS
    uint16_t *w = (uint16_t *)buf;
    for (int s=0;s<count;s++){
        wait_ready();
        if (wait_drq()) return;
        for (int i=0;i<256;i++) w[s*256+i] = inw(DATA);
    }
}
void ata_write(uint32_t lba, uint8_t count, const void *buf){
    setup(lba, count);
    outb(CMD, 0x30); //WRITE SECTORS
    const uint16_t *w = (const uint16_t *)buf;
    for (int s=0;s<count;s++){
        wait_ready();
        if (wait_drq()) return;
        for (int i=0;i<256;i++) outw(DATA, w[s*256+i]);
    }
    outb(CMD, 0xE7); //FLUSH CACHE
    wait_ready();
}