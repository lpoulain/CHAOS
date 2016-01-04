#ifndef __DISPLAY_H
#define __DISPLAY_H

#define WHITE_ON_BLACK 0x0f
#define YELLOW_ON_BLACK 0x0e

#define BUFFER_SIZE 128

// A display is a text window which can receive user input
typedef struct {
	unsigned char *start_address;
	unsigned char *end_address;
	unsigned char *cursor_address;
	char text_color;
	unsigned char buffer[BUFFER_SIZE];
	int buffer_end;
} display;

void print(const char *, int, int, char);
void print_hex(char b, int row, int col);
void print_hex2(char b, int row, int col);
void puts(display *disp, const char *);
void putc(display *disp, char);
void puti(display *disp, uint);
void putnb(display *disp, uint nb);
void cls(display *disp);
void putcr(display *disp);
void backspace(display *disp);
void set_cursor(display *disp);
void print_ptr(void *ptr, int row, int col);
void print_int(int n, int row, int col);
void dump_mem(void *ptr, int nb_bytes, int row);
void debug_i(char *msg, uint nb);
void debug(char *msg);

void init_display(display *disp, int row_start, int row_end, int color);

#endif
