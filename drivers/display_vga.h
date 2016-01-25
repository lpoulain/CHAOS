#ifndef __DISPLAY_VGA_H
#define __DISPLAY_VGA_H

#define VGA_ADDRESS 0xA0000

void init_vga();

void draw_cursor_buffer(uint x, uint y);
void save_cursor_buffer(uint x, uint y);

void draw_pixel(uint x, uint y);
void draw_font(unsigned char c, uint x, uint y);
void draw_cursor(uint x, uint y);
void draw_string(const unsigned char *str, int x, int y);
void draw_string_n(const unsigned char *str, int x, int y, int len);
void draw_box(uint left_x, uint right_x, uint top_y, uint bottom_y);
void draw_frame(int left_x, int right_x, int top_y, int bottom_y);
void draw_background(int left_x, int right_x, int top_y, int bottom_y);
void draw_hex(char b, int row, int col);
void draw_hex2(char b, int row, int col);
void draw_ptr(void *ptr, int row, int col);
void draw_font_inside_frame(unsigned char c, uint x, uint y, uint left_x, uint right_x, uint top_y, uint bottom_y);
void draw_string_inside_frame(const unsigned char *str, int x, int y, uint left_x, uint right_x, uint top_y, uint bottom_y);
void copy_box(uint left_x, uint right_x, uint top_y, uint bottom_y, uint nb_pixels_up);
void bitarray_copy(const unsigned char *src_org, int src_offset, int src_len,
                    unsigned char *dst_org, int dst_offset);

#endif