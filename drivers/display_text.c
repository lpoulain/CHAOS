#include "libc.h"
#include "kernel.h"
#include "display.h"
#include "display_text.h"

// Sets the cursor in the screen
void text_set_cursor(Window *win) {
	unsigned int offset = (unsigned int)(win->cursor_address - VIDEO_ADDRESS) / 2;

    outportb(0x3D4, 14);
    outportb(0x3D5, offset >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, offset);
}

// Scroll up the window one line
void text_scroll(Window *win) {
	int nb_characters = (win->end_address - win->start_address) / 2 - 80;
	unsigned char *ptr = win->start_address;

	for (int i=0; i<nb_characters; i++) {
		*ptr = *(ptr+160);
		ptr++;
		*ptr++ = win->text_color;
	}

	for (int i=0; i<80; i++) {
		*ptr = ' ';
		ptr+= 2;
	}

	win->cursor_address -= 160;
}

// Prints a string at a particular row and column on the screen
void text_print(const char *msg, int row, int col, char color) {
	int len = strlen(msg);
	
	unsigned char *video_memory = (unsigned char *)(VIDEO_ADDRESS + row * 160 + col * 2);
	int i;

	for (i=0; i<len; i++) {
		*video_memory++ = msg[i];
		*video_memory++ = color;
	}

}

// Clears the window
void text_cls(Window *win) {
	win->cursor_address = win->start_address;

	while (win->cursor_address < win->end_address) {
		*win->cursor_address++ = ' ';
		*win->cursor_address++ = win->text_color;
	}

	win->cursor_address = win->start_address;
}

// Prints a character in hex format, starting with the "0x"
void text_print_hex(char b, int row, int col) {
	unsigned char str[5];
	ctoa_hex_0x(b, (unsigned char *)&str);	
	text_print((unsigned char*)&str, row, col, YELLOW_ON_BLACK);
}

// Prints a character in hex format, without the "0x"
void text_print_hex2(char b, int row, int col) {
	unsigned char str[3];
	ctoa_hex(b, (unsigned char *)&str);
	text_print((unsigned char *)&str, row, col, YELLOW_ON_BLACK);
}

// Prints the value of a pointer (in hex) at a particular
// row and column on the screen
void text_print_ptr(void *ptr, int row, int col) {
	unsigned char str[11];
	itoa_hex_0x((uint)ptr, (unsigned char *)&str);
	text_print((unsigned char *)&str, row, col, YELLOW_ON_BLACK);
}

// Prints an integer at a particular row and column
// on the screen
void text_print_int(int nb, int row, int col) { 
	unsigned char str[12];
	itoa(nb, (unsigned char *)&str);
    text_print((unsigned char*)&str, row, col, YELLOW_ON_BLACK);
}

void text_print_c(char c, int row, int col) {
	unsigned char *video_memory = (unsigned char *)(VIDEO_ADDRESS + row * 160 + col * 2);

	*video_memory++ = c;
	*video_memory = YELLOW_ON_BLACK;
}

void init_text() {
	for (char *addr = (char*)VIDEO_ADDRESS; addr<(char*)VIDEO_ADDRESS + 160*50; addr += 2) {
		*addr = ' ';
		*(addr+1) = WHITE_ON_BLACK;
	}
	text_print("Welcome to CHAOS (CHeers, Another Operating System) !", 0, 0, WHITE_ON_BLACK);
}
