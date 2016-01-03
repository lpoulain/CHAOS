#ifndef __LIBC_H
#define __LIBC_H

typedef unsigned int   uint;
typedef          int   s32int;
typedef unsigned short u16int;
typedef          short s16int;
typedef unsigned char  u8int;
typedef          char  s8int;

char *strcpy(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
int strlen(const char *s);
int strncmp(const char *s1, const char *s2, uint n);

void memcpy(void *dest, const void *src, uint len);
void memset(void *dest, u8int val, uint len);

void getch();

void debug_i(char *msg, uint nb);
void debug(char *msg);
void C_stack_dump(void *esp, void *ebp);
extern void stack_dump();

#endif
