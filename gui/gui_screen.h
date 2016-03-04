#ifndef __GUI_SCREEN_H
#define __GUI_SCREEN_H

#include "font.h"

void draw_hex(char b, int x, int y);
void draw_hex2(char b, int x, int y);
void draw_ptr(void *ptr, int x, int y);
int draw_int(int nb, int x, int y);
void draw_string(const unsigned char *str, int x, int y);
void draw_string_n(const unsigned char *str, int x, int y, int len);
void draw_string_inside_frame(const unsigned char *str, int x, int y, uint left_x, uint right_x, uint top_y, uint bottom_y);
void draw_char(unsigned char, int, int);
void draw_char_inside_frame(unsigned char c, uint x, uint y, uint left_x, uint right_x, uint top_y, uint bottom_y);
void draw_proportional_string(const unsigned char *str, uint8 font_idx, uint x, uint y);
void draw_proportional_string_inside_frame(const unsigned char *str, uint8 font_idx, int x, int y, uint left_x, uint right_x, uint top_y, uint bottom_y);
int get_proportional_string_length(const unsigned char *str, uint8 font_idx);

extern Window gui_debug_win;

#endif
