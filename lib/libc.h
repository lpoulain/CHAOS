#ifndef __LIBC_H
#define __LIBC_H

char *strcpy(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
int strlen(const char *s);

void *memset(void *dest, char val, size_t count);

#endif
