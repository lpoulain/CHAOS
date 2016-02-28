// Functions that handle the VGA mouse

#include "libc.h"
#include "display.h"
#include "display_vga.h"
#include "gui_mouse.h"
#include "gui_window.h"

static int mouse_x = 320;
static int mouse_y = 200;

Window *window_focus = 0;

uint window_copy_buffer[20*480];

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
	draw_mouse_cursor_buffer(mouse_x, mouse_y);

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
	save_mouse_cursor_buffer(mouse_x, mouse_y);
	draw_mouse_cursor(mouse_x, mouse_y);
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
	draw_mouse_cursor_buffer(mouse_x, mouse_y);

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

	// First copy the screen to a buffer
	// In order to deal only with 32-bit, we copy whole lines of pixels
	uint *src1 = (uint*)(VGA_ADDRESS + 80*window_focus->top_y);
	lmemcpy((uint*)&window_copy_buffer, src1, 20*(window_focus->bottom_y - window_focus->top_y + 1));


	// Refreshes the two rectangles which are not hidden anymore by the window
	// For now just draws the background

	// We need to redraw what is behind the old position
	// There is of course the background, but also the other window
	rect redraw_background[2];
	rect redraw_other_window[2];
	int nb_rects_to_redraw = 0;

	// First step: redraw the background
	int y_from, y_to;
	if (delta_y > 0) {
		draw_background(window_focus->left_x, window_focus->right_x, window_focus->top_y, window_focus_y - 1);
		y_from = window_focus_y;
		y_to = window_focus->bottom_y;

		redraw_background[0].left_x = window_focus->left_x;
		redraw_background[0].right_x = window_focus->right_x;
		redraw_background[0].top_y = window_focus->top_y;
		redraw_background[0].bottom_y = window_focus_y - 1;
	} else {
		draw_background(window_focus->left_x, window_focus->right_x, window_focus_y + window_focus_height, window_focus->bottom_y);
		y_from = window_focus->top_y;
		y_to = window_focus_y + window_focus_height;

		redraw_background[0].left_x = window_focus->left_x;
		redraw_background[0].right_x = window_focus->right_x;
		redraw_background[0].top_y = window_focus_y + window_focus_height;
		redraw_background[0].bottom_y = window_focus->bottom_y;
	}
	if (delta_x > 0) {
		draw_background(window_focus->left_x, window_focus_x - 1, y_from, y_to);

		redraw_background[1].left_x = window_focus->left_x;
		redraw_background[1].right_x = window_focus_x - 1;
		redraw_background[1].top_y = y_from;
		redraw_background[1].bottom_y = y_to;
	} else {
		draw_background((uint)(window_focus_x + window_focus_width), (uint)(window_focus->right_x), y_from, y_to);

		redraw_background[1].left_x = window_focus_x + window_focus_width;
		redraw_background[1].right_x = window_focus->right_x;
		redraw_background[1].top_y = y_from;
		redraw_background[1].bottom_y = y_to;
	}


	// Then do a copy to the target
   	unsigned char *src = (unsigned char *)&window_copy_buffer + window_focus->left_x/8;
   	unsigned char *dst = (unsigned char *)VGA_ADDRESS + 80*window_focus_y + window_focus_x/8;

    for (i=window_focus->top_y; i<=window_focus->bottom_y; i++) {
	    bitarray_copy((const unsigned char *)src, window_focus->left_x % 8, window_focus->right_x - window_focus->left_x + 1,
		    			  dst, window_focus_x % 8);
	    src += 80;
	    dst += 80;
	}


	// Second step: redraw the other window
	Window *other_window;
	if (window_focus == &gui_win1) other_window = &gui_win2;
	else other_window = &gui_win1;
	rect other_window_rect = {
								.left_x = other_window->left_x,
								.right_x = other_window->right_x,
								.top_y = other_window->top_y,
								.bottom_y = other_window->bottom_y
							 };
	for (i=0; i<2; i++) {
		nb_rects_to_redraw += intersection(&redraw_background[i], &other_window_rect, &redraw_other_window[nb_rects_to_redraw]);
	}

	if (nb_rects_to_redraw > 0) 

	for (i=0; i<nb_rects_to_redraw; i++) {
		other_window->action->redraw(other_window,
						   redraw_other_window[i].left_x,
						   redraw_other_window[i].right_x,
						   redraw_other_window[i].top_y,
						   redraw_other_window[i].bottom_y);
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
	save_mouse_cursor_buffer(mouse_x, mouse_y);
	draw_mouse_cursor(mouse_x, mouse_y);		
}

void gui_mouse_hide() {
	draw_mouse_cursor_buffer(mouse_x, mouse_y);
}

void gui_mouse_show() {
	save_mouse_cursor_buffer(mouse_x, mouse_y);
	draw_mouse_cursor(mouse_x, mouse_y);
}

void gui_save_mouse_buffer() {
	save_mouse_cursor_buffer(mouse_x, mouse_y);
}
