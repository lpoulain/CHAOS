#ifndef __LIBC_H
#define __LIBC_H

typedef unsigned int   uint;
typedef          int   sint32;
typedef unsigned short uint16;
typedef          short sint16;
typedef unsigned char  uint8;
typedef          char  sint8;

char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, int n);
int strcmp(const char *s1, const char *s2);
int strlen(const char *s);
int strnlen (const char *s, uint maxlen);
int strncmp(const char *s1, const char *s2, uint n);

uint atoi_hex(char *str);
int atoi(char *str);
void itoa(int, char*);

void printf(const char *format, ...);

uint umin(uint nb1, uint nb2);
uint umax(uint nb1, uint nb2);
int min(int nb1, int nb2);
int max(int nb1, int nb2);

void memcpy(void *dest, const void *src, uint len);
void memset(void *dest, uint8 val, uint len);
void lmemcpy(uint *dest, const uint *src, uint len);
void lmemset(uint *dest, uint val, uint len);
void text_dump_mem(void *ptr, int nb_bytes, int row);
void gui_dump_mem(void *ptr, int nb_bytes, int row);

unsigned char getch();

void debug_i(char *msg, uint nb);
void debug(char *msg);
void C_stack_dump(void *esp, void *ebp);
extern void stack_dump();

#endif
