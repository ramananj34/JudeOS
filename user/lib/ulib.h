#pragma once
typedef unsigned long size_t;
long write(int fd, const void *buf, size_t len);
long read(int fd, void *buf, size_t len);
int getpid(void);
void exit(int code);
size_t strlen(const char *s);
int streq(const char *a, const char *b);
void puts(const char *s);
int readline(char *buf, int max);
int open(const char *path);
int close(int fd);