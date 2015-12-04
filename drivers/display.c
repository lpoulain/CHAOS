#include "kernel.h"
#include "libc.h"
#include "display.h"

#define VIDEO_ADDRESS 0xb8000
#define VIDEO_ADDRESS_END 0xb8fa0

int cursor_row = 0;
int cursor_col = 0;
char text_color=WHITE_ON_BLACK;

unsigned char *video_memory = (unsigned char *)VIDEO_ADDRESS;

void set_cursor() {
	unsigned int offset = (unsigned int)(video_memory - VIDEO_ADDRESS) / 2;

    outportb(0x3D4, 14);
    outportb(0x3D5, offset >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, offset);
}

void scroll() {
	int nb_characters = 80 * 24;
	unsigned char *ptr = (unsigned char *)VIDEO_ADDRESS;

	for (int i=0; i<nb_characters; i++) {
		*ptr = *(ptr+160);
		ptr+= 2;
	}

	for (int i=0; i<80; i++) {
		*ptr = ' ';
		ptr+= 2;
	}

	video_memory -= 160;
	set_cursor();
}

void putcr() {
	int offset = (unsigned int)(video_memory - VIDEO_ADDRESS);

	// If we are at the end, scroll
	if (offset >= 24 * 160) {
		scroll();
	}

	offset = offset % 160;
	video_memory+= 160 - offset;
	set_cursor();
}

void putc(char c) {
	if (c == '\n') {
		putcr();
	} else {
		if (video_memory >= (unsigned char *)VIDEO_ADDRESS_END) scroll();

		*video_memory++ = c;
		*video_memory++ = text_color;
	}
	set_cursor();
}

void backspace() {
	// We don't want to go before the video buffer
	if (video_memory == (unsigned char *)VIDEO_ADDRESS) return;
	video_memory-=2;
	*video_memory = ' ';
	set_cursor();	
}

void puts(const char *text) {
	while (*text) putc(*text++);
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

void cls() {
	video_memory = (unsigned char *)VIDEO_ADDRESS;
	int nb_characters = 80 * 25;

	while (nb_characters > 0) {
		*video_memory++ = ' ';
		video_memory++;
		nb_characters--;
	}

	video_memory = (unsigned char *)VIDEO_ADDRESS;
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
