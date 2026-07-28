#include "ulib.h"
static long sys(long n, long a, long b, long c) {
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c) : "rcx", "r11", "memory");
    return r;
}
long write(int fd, const void *buf, size_t len) { return sys(1, fd, (long)buf, len); }
long read (int fd, void *buf, size_t len) { return sys(3, fd, (long)buf, len); }
int  getpid(void) { return (int)sys(2, 0, 0, 0); }
void exit(int code) { sys(0, code, 0, 0); for(;;); }

size_t strlen(const char *s){ size_t n=0; while(s[n]) n++; return n; }
int streq(const char *a, const char *b){ while(*a && *a==*b){a++;b++;} return *a==*b; }
void puts(const char *s){ write(1, s, strlen(s)); }

int readline(char *buf, int max){
    int i = 0;
    for(;;){
        char c;
        while (read(0, &c, 1) <= 0) { } //poll the serial line
        if (c=='\r' || c=='\n'){ buf[i]=0; write(1,"\n",1); return i; }
        if (c==0x7f || c==8){ if(i>0){ i--; write(1,"\b \b",3); } continue; }
        if (i < max-1){ buf[i++]=c; write(1,&c,1); } //echo
    }
}
int open(const char *path){ return (int)sys(4, (long)path, 0, 0); }
int close(int fd){ return (int)sys(5, fd, 0, 0); }