#ifndef __DISPLAY_TEXT_H
#define __DISPLAY_TEXT_H

#include "display.h"

#define VIDEO_ADDRESS 0xb8000
#define VIDEO_ADDRESS_END 0xb8fa0

void text_set_cursor(window *win);
void text_scroll(window *win);
void text_print(const char *msg, int row, int col, char color);
void text_cls(window *win);

void text_print_hex(char b, int row, int col);
void text_print_hex2(char b, int row, int col);
void text_print_ptr(void *ptr, int row, int col);
void text_print_int(int n, int row, int col);
void text_print_c(char c, int row, int col);

void init_text();

#endif
