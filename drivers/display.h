#ifndef __DISPLAY_H
#define __DISPLAY_H

#define WHITE_ON_BLACK 0x0f
#define YELLOW_ON_BLACK 0x0e

void print(const char *, int, int, char);
void print_hex(char b, int row, int col);
void puts(const char *);
void putc(char);
void cls();
void putcr();
void backspace();

#endif
