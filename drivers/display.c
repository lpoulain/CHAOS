#include "libc.h"
#include "kernel.h"
#include "display.h"

#define VIDEO_ADDRESS 0xb8000
#define VIDEO_ADDRESS_END 0xb8fa0

#define PROCESS_STACK_SIZE 16384

// Sets the cursor in the screen
void set_cursor(display *disp) {
	unsigned int offset = (unsigned int)(disp->cursor_address - VIDEO_ADDRESS) / 2;

    outportb(0x3D4, 14);
    outportb(0x3D5, offset >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, offset);
}

// Scroll up the display one line
void scroll(display *disp) {
	int nb_characters = (disp->end_address - disp->start_address) / 2 - 80;
	unsigned char *ptr = disp->start_address;

	for (int i=0; i<nb_characters; i++) {
		*ptr = *(ptr+160);
		ptr++;
		*ptr++ = disp->text_color;
	}

	for (int i=0; i<80; i++) {
		*ptr = ' ';
		ptr+= 2;
	}

	disp->cursor_address -= 160;
}

// Prints a carriage return on the display
void putcr(display *disp) {
	if (disp->cursor_address == disp->end_address) {
		scroll(disp);
	}

	uint offset = (disp->cursor_address - disp->start_address) % 160;
	disp->cursor_address += 160 - offset;
}

// Prints a character on the display
void putc(display *disp, char c) {
	if (c == '\n') {
		putcr(disp);
	} else {
		if (disp->cursor_address >= disp->end_address) scroll(disp);

		*disp->cursor_address++ = c;
		*disp->cursor_address++ = disp->text_color;
	}
}

// Prints an integer (in hex format) on the display
void puti(display *disp, uint i) {
	unsigned char *addr = (unsigned char *)&i;
	char *str = "0x00000000";
	char *loc = str++;

	char key[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	addr += 3;

	for (int i=0; i<4; i++) {
		unsigned char b = *addr--;
		int u = ((b & 0xf0) >> 4);
		int l = (b & 0x0f);
		*++str = key[u];
		*++str = key[l];
	}

	puts(disp, loc);
}

// Prints a number on the display
void putnb(display *disp, int nb) {
	if (nb < 0) {
		putc(disp, '-');
		nb = -nb;
	}
	uint nb_ref = 1000000000;
	uint leading_zero = 1;
	uint digit;

	for (int i=0; i<=9; i++) {
		if (nb >= nb_ref) {
			digit = nb / nb_ref;
			putc(disp, '0' + digit);
			nb -= nb_ref * digit;

			leading_zero = 0;
		} else {
			if (!leading_zero) putc(disp, '0');
		}
		nb_ref /= 10;
	}
}

// When the user presses backspace
void backspace(display *disp) {
	// We don't want to go before the video buffer
	if (disp->cursor_address == disp->start_address) return;
	disp->cursor_address -= 2;
	*disp->cursor_address = ' ';
}

// Prints a string on the display
void puts(display *disp, const char *text) {
	while (*text) putc(disp, *text++);
}

// Prints a string at a particular row and column on the screen
void print(const char *msg, int row, int col, char color) {
	int len = strlen(msg);
	
	unsigned char *video_memory = (unsigned char *)(VIDEO_ADDRESS + row * 160 + col * 2);
	int i;

	for (i=0; i<len; i++) {
		*video_memory++ = msg[i];
		*video_memory++ = color;
	}

}

// Clears the display
void cls(display *disp) {
	disp->cursor_address = disp->start_address;

	while (disp->cursor_address < disp->end_address) {
		*disp->cursor_address++ = ' ';
		*disp->cursor_address++ = disp->text_color;
	}

	disp->cursor_address = disp->start_address;
}

void init_screen() {
	for (char *addr = (char*)VIDEO_ADDRESS; addr<(char*)VIDEO_ADDRESS + 160*50; addr += 2) {
		*addr = ' ';
		*(addr+1) = WHITE_ON_BLACK;
	}
	print("Welcome to CHAOS (CHeers, Another Operating System) !", 0, 0, WHITE_ON_BLACK);
}

// Prints a character in hex format, starting with the "0x"
void print_hex(char b, int row, int col) {
	char* str = "0x00"; //placeholder for size
	char* loc = str++; //offset since beginning has 0x
	
	char key[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	int u = ((b & 0xf0) >> 4);
	int l = (b & 0x0f);
	*++str = key[u];
	*++str = key[l];

	//*++str = '\n'; //newline
	print(loc, row, col, YELLOW_ON_BLACK);
}

// Prints a character in hex format, without the "0x"
void print_hex2(char b, int row, int col) {
	char* str = "00"; //placeholder for size
	char* loc = str; //offset since beginning has 0x
	
	char key[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	int u = ((b & 0xf0) >> 4);
	int l = (b & 0x0f);
	*str++ = key[u];
	*str++ = key[l];

	//*++str = '\n'; //newline
	print(loc, row, col, YELLOW_ON_BLACK);
}

// Prints the value of a pointer (in hex) at a particular
// row and column on the screen
void print_ptr(void *ptr, int row, int col) {
	unsigned char *addr = (unsigned char *)&ptr;
	char *str = "0x00000000";
	char *loc = str++;

	char key[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	addr += 3;

	for (int i=0; i<4; i++) {
		unsigned char b = *addr--;
		int u = ((b & 0xf0) >> 4);
		int l = (b & 0x0f);
		*++str = key[u];
		*++str = key[l];
	}

	print(loc, row, col, YELLOW_ON_BLACK);
}

// Prints an integer at a particular row and column
// on the screen
void print_int(int n, int row, int col)
{ 
	char *str = "           ";
	char *loc = str;

	if (n == 0) *loc++ = '0';

    while( n > 0 ) {
    	*loc++ = '0' + (n % 10);
    	n /= 10;
    }

    print(str, row, col, YELLOW_ON_BLACK);
}

void print_c(char c, int row, int col) {
	unsigned char *video_memory = (unsigned char *)(VIDEO_ADDRESS + row * 160 + col * 2);

	*video_memory++ = c;
	*video_memory = YELLOW_ON_BLACK;
}

void init_display(display *disp, int row_start, int row_end, int color) {
	disp->start_address = (unsigned char*)VIDEO_ADDRESS + row_start * 160;
	disp->end_address = (unsigned char*)VIDEO_ADDRESS + (row_end + 1) * 160;
	disp->cursor_address = disp->start_address;
	disp->text_color = color;
	disp->buffer_end = 0;
}
