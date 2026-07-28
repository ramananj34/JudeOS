#include "lib/ulib.h"
static int startswith(const char *s, const char *p){ while(*p){ if(*s++ != *p++) return 0; } return 1; }
int main(void){
    char line[128];
    puts("\n== mini-shell (ring 3) ==\ncommands: help, echo <text>, pid, exit\n");
    for(;;){
        puts("$ ");
        readline(line, sizeof line);
        if (streq(line, "help")) puts("help | echo <text> | pid | exit\n");
        else if (streq(line, "pid")){ char b[8]="pid ?\n"; b[4]='0'+getpid(); puts(b); }
        else if (startswith(line, "echo ")){ puts(line+5); puts("\n"); }
        else if (streq(line, "exit")){ puts("bye\n"); exit(0); }
        else if (streq(line, "")){ }
        else puts("unknown command\n");
    }
}