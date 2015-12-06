#ifndef __DISPLAY_H
#define __DISPLAY_H

#define WHITE_ON_BLACK 0x0f
#define YELLOW_ON_BLACK 0x0e



void print(const char *, int, int, char);
void print_hex(char b, int row, int col);
void puts(struct display *disp, const char *);
void putc(struct display *disp, char);
void cls(struct display *disp);
void putcr(struct display *disp);
void backspace(struct display *disp);
void set_cursor(struct display *disp);

void display_initialize(struct display *disp, int row_start, int row_end, int color);

#endif
