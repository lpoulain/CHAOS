// Functions to draw on the screen (outside of windows)

#include "libc.h"
#include "display.h"
#include "vga.h"

void draw_hex(char b, int x, int y) {
	char* str = "0x00"; //placeholder for size
	char* loc = str++; //offset since beginning has 0x
	
	char key[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	int u = ((b & 0xf0) >> 4);
	int l = (b & 0x0f);
	*++str = key[u];
	*++str = key[l];

	//*++str = '\n'; //newline
	draw_string(loc, x, y);
}

// Prints a character in hex format, without the "0x"
void draw_hex2(char b, int x, int y) {
	char* str = "00"; //placeholder for size
	char* loc = str; //offset since beginning has 0x
	
	char key[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	int u = ((b & 0xf0) >> 4);
	int l = (b & 0x0f);
	*str++ = key[u];
	*str++ = key[l];

	//*++str = '\n'; //newline
	draw_string(loc, x, y);
}

// Prints the value of a pointer (in hex) at a particular
// row and column on the screen
void draw_ptr(void *ptr, int x, int y) {
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

	draw_string(loc, x, y);
}
