#include "kernel.h"
#include "libc.h"
#include "display.h"

#define VIDEO_ADDRESS 0xb8000
#define VIDEO_ADDRESS_END 0xb8fa0


void set_cursor(struct display *disp) {
	unsigned int offset = (unsigned int)(disp->cursor_address - VIDEO_ADDRESS) / 2;

    outportb(0x3D4, 14);
    outportb(0x3D5, offset >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, offset);
}

void scroll(struct display *disp) {
	int nb_characters = (disp->end_address - disp->start_address) / 2 - 80;
	unsigned char *ptr = disp->start_address;

	for (int i=0; i<nb_characters; i++) {
		*ptr = *(ptr+160);
		ptr++;
		*ptr++ - disp->text_color;
	}

	for (int i=0; i<80; i++) {
		*ptr = ' ';
		ptr+= 2;
	}

	disp->cursor_address -= 160;
	set_cursor(disp);
}

void putcr(struct display *disp) {
	if (disp->cursor_address == disp->end_address) {
		scroll(disp);
	}

	uint offset = (disp->cursor_address - disp->start_address) % 160;
	disp->cursor_address += 160 - offset;
	set_cursor(disp);
}

void putc(struct display *disp, char c) {
	if (c == '\n') {
		putcr(disp);
	} else {
		if (disp->cursor_address >= disp->end_address) scroll(disp);

		*disp->cursor_address++ = c;
		*disp->cursor_address++ = disp->text_color;
	}
	set_cursor(disp);
}

void backspace(struct display *disp) {
	// We don't want to go before the video buffer
	if (disp->cursor_address == disp->start_address) return;
	disp->cursor_address -= 2;
	*disp->cursor_address = ' ';
	set_cursor(disp);
}

void puts(struct display *disp, const char *text) {
	while (*text) putc(disp, *text++);
}

void print(const char *msg, int row, int col, char color) {
	int len = strlen(msg);
	
	unsigned char *video_memory = (unsigned char *)(VIDEO_ADDRESS + row * 160 + col * 2);
	int i;

	for (i=0; i<len; i++) {
		*video_memory++ = msg[i];
		*video_memory++ = color;
	}

}

void cls(struct display *disp) {
	disp->cursor_address = disp->start_address;

	while (disp->cursor_address < disp->end_address) {
		*disp->cursor_address++ = ' ';
		*disp->cursor_address++ = disp->text_color;
	}

	disp->cursor_address = disp->start_address;
}


unsigned short cursor_pos = 0;

void print_next(char *str)
{
	unsigned char *video_memory = (unsigned char *)(VIDEO_ADDRESS + cursor_pos * 2);

	while (*str)
	{
		*video_memory = *str;
		cursor_pos++;
		str++;
	}	
}


void print_hex(char b, int row, int col){
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

void display_initialize(struct display *disp, int row_start, int row_end, int color) {
	disp->start_address = (unsigned char*)VIDEO_ADDRESS + row_start * 160;
	disp->end_address = (unsigned char*)VIDEO_ADDRESS + (row_end + 1) * 160;
	disp->cursor_address = disp->start_address;
	disp->text_color = color;
}
