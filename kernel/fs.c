#include "fs.h"
#include "ata.h"

struct __attribute__((packed)) entry { char name[56]; uint32_t start_lba, size; };
static struct entry files[7];
static int nfiles;

static int kstreq(const char *a, const char *b){ while(*a && *a==*b){a++;b++;} return *a==*b; }

void fs_init(void){
    uint8_t sec[512];
    ata_read(256, 1, sec);
    if (*(uint32_t *)sec != 0x53465331) { nfiles = 0; return; } //magic 'SFS1'
    uint32_t n = *(uint32_t *)(sec + 4);
    if (n > 7) n = 7;
    for (uint32_t i = 0; i < n; i++)
        for (int k = 0; k < 64; k++) ((uint8_t *)&files[i])[k] = sec[8 + i*64 + k];
    nfiles = n;
}
int fs_open(const char *name){
    for (int i = 0; i < nfiles; i++) if (kstreq(files[i].name, name)) return i;
    return -1;
}
uint32_t fs_size(int idx){ return (idx>=0 && idx<nfiles) ? files[idx].size : 0; }

int fs_read(int idx, uint32_t off, void *buf, uint32_t len){
    if (idx < 0 || idx >= nfiles) return -1;
    uint32_t size = files[idx].size;
    if (off >= size) return 0;
    if (off + len > size) len = size - off;
    uint8_t tmp[512];
    uint32_t done = 0;
    while (done < len){
        uint32_t pos = off + done;
        ata_read(files[idx].start_lba + pos/512, 1, tmp);
        uint32_t so = pos % 512, chunk = 512 - so;
        if (chunk > len - done) chunk = len - done;
        for (uint32_t k = 0; k < chunk; k++) ((uint8_t *)buf)[done+k] = tmp[so+k];
        done += chunk;
    }
    return len;
}