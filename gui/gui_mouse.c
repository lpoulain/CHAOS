// Functions that handle the VGA mouse

#include "libc.h"
#include "display.h"
#include "vga.h"
#include "gui_mouse.h"
#include "gui_window.h"

static int mouse_x = 320;
static int mouse_y = 200;

window *window_focus = 0;

int window_focus_x;
int window_focus_y;
int window_focus_width;
int window_focus_height;

uint frame_buffer_top[20] = {0x123456};
uint frame_buffer_bottom[20];
uint frame_buffer_left[480];
uint frame_buffer_right[480];

// Move the mouse cursor
void gui_mouse_move(int delta_x, int delta_y, uint mouse_button_down) {
	if (delta_x == 0 && delta_y == 0) return;

	int i;
	uint *address_left, *address_right, *address_top, *address_bottom;
	draw_cursor_buffer(mouse_x, mouse_y);

	// If we have the button pressed down and the cursor on a window
	// move the window
	if (mouse_button_down && window_focus) {
		// Restore the frame buffer on the screen
		// i.e. erase the previous frame
		if (frame_buffer_top[0] != 0x123456) {
			address_left = (uint*)VGA_ADDRESS + 20*window_focus_y + window_focus_x/32;
			address_right = (uint*)VGA_ADDRESS + 20*window_focus_y + (window_focus_x + window_focus_width - 1)/32;

			for (i=0; i<window_focus_height; i++) {
				*address_left = frame_buffer_left[i];
				*address_right = frame_buffer_right[i];
				address_left += 20;
				address_right += 20;
			}

			address_top = (uint*)VGA_ADDRESS + 20*window_focus_y + window_focus_x/32;
			address_bottom = (uint*)VGA_ADDRESS + 20*(window_focus_y + window_focus_height - 1) + window_focus_x/32;
			for (i=0; i<20; i++) {
				*address_top++ = frame_buffer_top[i];
				*address_bottom++ = frame_buffer_bottom[i];
			}
		}

		// Moves the frame
		window_focus_x += delta_x;
		window_focus_y += delta_y;
		// To simplify things, we don't let the window get outside the screen
		if (window_focus_x < 0) window_focus_x = 0;
		if (window_focus_x + window_focus_width - 1 >= 640) window_focus_x = 640 - window_focus_width;
		if (window_focus_y < 0) window_focus_y = 0;
		if (window_focus_y + window_focus_height - 1 >= 480) window_focus_y = 480 - window_focus_height;

		// Saves the new frame buffer
		address_left = (uint*)VGA_ADDRESS + 20*window_focus_y + window_focus_x/32;
		address_right = (uint*)VGA_ADDRESS + 20*window_focus_y + (window_focus_x + window_focus_width - 1)/32;

		for (i=0; i<window_focus_height; i++) {
			frame_buffer_left[i] = *address_left;
			frame_buffer_right[i] = *address_right;
			address_left += 20;
			address_right += 20;
		}

		address_top = (uint*)VGA_ADDRESS + 20*window_focus_y + window_focus_x/32;
		address_bottom = (uint*)VGA_ADDRESS + 20*(window_focus_y + window_focus_height - 1) + window_focus_x/32;
		for (i=0; i<20; i++) {
			frame_buffer_top[i] = *address_top++;
			frame_buffer_bottom[i] = *address_bottom++;
		}

		// Draw the frame
		draw_frame(window_focus_x, window_focus_x + window_focus_width - 1,
				   window_focus_y, window_focus_y + window_focus_height - 1);
	}

	// Move the mouse cursor
	mouse_x += delta_x;
	mouse_y += delta_y;
	// Make sure the cursor tip does not leave the screen
	if (mouse_x < 0) mouse_x = 0;
	if (mouse_x >= 640) mouse_x = 639;
	if (mouse_y < 0) mouse_y = 0;
	if (mouse_y >= 480) mouse_y = 479;

	// Displays the mouse
	save_cursor_buffer(mouse_x, mouse_y);
	draw_cursor(mouse_x, mouse_y);
}

void gui_mouse_click() {
	window_focus = gui_handle_mouse_click(mouse_x, mouse_y);
	window_focus_x = window_focus->left_x;
	window_focus_y = window_focus->top_y;
	window_focus_width = window_focus->right_x + 1 - window_focus->left_x;
	window_focus_height = window_focus->bottom_y + 1 - window_focus->top_y;
}

// The user releases the mouse button
void gui_mouse_unclick() {
	// If the frame buffer is filled, it means
	// we were moving a window. Time to redraw it
	if (frame_buffer_top[0] == 0x123456) return;

	// Computes the delta
	int i;
	int delta_x = window_focus_x - window_focus->left_x;
	int delta_y = window_focus_y - window_focus->top_y;

	// The window was not moved. Stop right here.
	if (delta_x == 0 && delta_y == 0) {
		frame_buffer_top[0] == 0x123456;
		return;
	}

	// Hides the mouse
	draw_cursor_buffer(mouse_x, mouse_y);

	// Hides the frame (restores the frame buffer)
	uint *address_left = (uint*)VGA_ADDRESS + 20*window_focus_y + window_focus_x/32;
	uint *address_right = (uint*)VGA_ADDRESS + 20*window_focus_y + (window_focus_x + window_focus_width - 1)/32;

	for (i=0; i<window_focus_height; i++) {
		*address_left = frame_buffer_left[i];
		*address_right = frame_buffer_right[i];
		address_left += 20;
		address_right += 20;
	}

	uint *address_top = (uint*)VGA_ADDRESS + 20*window_focus_y + window_focus_x/32;
	uint *address_bottom = (uint*)VGA_ADDRESS + 20*(window_focus_y + window_focus_height - 1) + window_focus_x/32;
	for (i=0; i<20; i++) {
		*address_top++ = frame_buffer_top[i];
		*address_bottom++ = frame_buffer_bottom[i];
	}

	// Copies the window to the new place
	// Because we are copying what is on the screen, we need to make sure we are copying
	// from either top to bottom or bottom to top, depending on the case
	// The only time this does not work is when we copy the window to the left
	// In a future versions we might want to ask the process to redraw the window
   	if (delta_y < 0) {
	   	unsigned char *src = (unsigned char *)VGA_ADDRESS + 80*window_focus->top_y + window_focus->left_x/8;
	   	unsigned char *dst = (unsigned char *)VGA_ADDRESS + 80*window_focus_y + window_focus_x/8;

	    for (i=window_focus->top_y; i<=window_focus->bottom_y; i++) {
		    bitarray_copy((const unsigned char *)src, window_focus->left_x % 8, window_focus->right_x - window_focus->left_x + 1,
		    			  dst, window_focus_x % 8);
		    src += 80;
		    dst += 80;
		}
	} else {
	   	unsigned char *src = (unsigned char *)VGA_ADDRESS + 80*window_focus->bottom_y + window_focus->left_x/8;
	   	unsigned char *dst = (unsigned char *)VGA_ADDRESS + 80*(window_focus_y + window_focus_height - 1) + window_focus_x/8;

	    for (i=window_focus->bottom_y; i>= window_focus->top_y; i--) {
		    bitarray_copy((const unsigned char *)src, window_focus->left_x % 8, window_focus->right_x - window_focus->left_x + 1,
		    			  dst, window_focus_x % 8);
		    src -= 80;
		    dst -= 80;
		}
	}

	// Refreshes the two rectangles which are not hidden anymore by the window
	// For now just draws the background

	int y_from, y_to;
	if (delta_y > 0) {
		draw_background(window_focus->left_x, window_focus->right_x, window_focus->top_y, window_focus_y - 1);
		y_from = window_focus_y;
		y_to = window_focus->bottom_y;
	} else {
		draw_background(window_focus->left_x, window_focus->right_x, window_focus_y + window_focus_height, window_focus->bottom_y);
		y_from = window_focus->top_y;
		y_to = window_focus_y + window_focus_height;
	}
	if (delta_x > 0) {
		draw_background(window_focus->left_x, window_focus_x - 1, y_from, y_to);
	} else {
		draw_background((uint)(window_focus_x + window_focus_width), (uint)(window_focus->right_x), y_from, y_to);
	}

	// Sets the new window settings
	window_focus->left_x += delta_x;
	window_focus->right_x += delta_x;
	window_focus->top_y += delta_y;
	window_focus->bottom_y += delta_y;
	window_focus->cursor_x += delta_x;
	window_focus->cursor_y += delta_y;

	frame_buffer_top[0] = 0x123456;

	// Shows the mouse cursor
	save_cursor_buffer(mouse_x, mouse_y);
	draw_cursor(mouse_x, mouse_y);		
}

void gui_mouse_hide() {
	draw_cursor_buffer(mouse_x, mouse_y);
}

void gui_mouse_show() {
	save_cursor_buffer(mouse_x, mouse_y);
	draw_cursor(mouse_x, mouse_y);
}

void gui_save_mouse_buffer() {
	save_cursor_buffer(mouse_x, mouse_y);
}
