#include "libc.h"
#include "display.h"
#include "display_text.h"

static uint8 cursor_buffer = ' ';
static uint8 cursor_color_buffer = 0;

static int mouse_x = 40;
static int mouse_y = 12;

void text_mouse_move(int delta_x, int delta_y) {
//	debug_i("X: ", delta_x);
	delta_x /= 2;
	delta_y /= 2;

	unsigned char *video_memory = (unsigned char *)(VIDEO_ADDRESS + mouse_y * 160 + mouse_x * 2);
	*video_memory++ = cursor_buffer;
	*video_memory = cursor_color_buffer;

	mouse_x += delta_x;
	mouse_y += delta_y;

	if (mouse_x < 0) mouse_x = 0;
	if (mouse_x >= 80) mouse_x = 79;
	if (mouse_y < 0) mouse_y = 0;
	if (mouse_y >= 25) mouse_y = 24;

	video_memory = (unsigned char *)(VIDEO_ADDRESS + mouse_y * 160 + mouse_x * 2);
	cursor_buffer = *video_memory;
	*video_memory++ = 'X';
	cursor_color_buffer = *video_memory;
	*video_memory = YELLOW_ON_BLACK;
}

void text_mouse_click() { }
void text_mouse_unclick() { }

void text_mouse_show() {
	unsigned char *video_memory = (unsigned char *)(VIDEO_ADDRESS + mouse_y * 160 + mouse_x * 2);
	cursor_buffer = *video_memory;
	*video_memory++ = 'X';
	cursor_color_buffer = *video_memory;
	*video_memory = YELLOW_ON_BLACK;
}

void text_mouse_hide() {
	unsigned char *video_memory = (unsigned char *)(VIDEO_ADDRESS + mouse_y * 160 + mouse_x * 2);
	*video_memory++ = cursor_buffer;
	*video_memory = cursor_color_buffer;
}
