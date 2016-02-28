#include "libc.h"
#include "kheap.h"
#include "display.h"
#include "display_vga.h"
//#include "process.h"
#include "gui_screen.h"
#include "gui_window.h"
#include "gui_mouse.h"

//extern process processes[3];

// Given two rectangles, compute the intersection
int intersection(rect *rect1, rect *rect2, rect *intersect) {

	// If the two rectangles do not intersect, return null
	if (rect1->right_x < rect2->left_x ||
		rect2->right_x < rect1->left_x ||
		rect1->bottom_y < rect2->top_y ||
		rect2->bottom_y < rect1->top_y)
		return 0;

	intersect->left_x = umax(rect1->left_x, rect2->left_x);
	intersect->right_x = umin(rect1->right_x, rect2->right_x);
	intersect->top_y = umax(rect1->top_y, rect2->top_y);
	intersect->bottom_y = umin(rect1->bottom_y, rect2->bottom_y);

	return 1;
}

///////////////////////////////////////////////////////////////
// GUI Window functions
///////////////////////////////////////////////////////////////
void gui_redraw_frame(Window *win, uint left_x, uint right_x, uint top_y, uint bottom_y);

// We've reached the bottom of the window, we need to scroll everything up one line
void gui_scroll(Window *win) {
	copy_box(win->left_x+1, win->right_x-1, win->top_y + 10, win->cursor_y - 8, 8);
	draw_box(win->left_x+1, win->right_x-1, win->cursor_y - 7, win->cursor_y);
	win->cursor_y -= 8;

	memcpy(win->text + 80, win->text, BUFFER_SIZE - 80);
	win->text_end -= 80;
}

void gui_scroll_down(Window *win, uint y) {
	uint bottom = (win->top_y + 11) + 8*((win->bottom_y-1 - (win->top_y + 11)) / 8);
	copy_box(win->left_x+1, win->right_x-1, win->top_y + 11 + y*8, bottom, -8);
	draw_box(win->left_x+1, win->right_x-1, win->top_y + 11 + y*8, win->top_y + 17 + y*8);
//	draw_frame(win->left_x+1, win->right_x-1, win->top_y + 11 + y*8, bottom - 8);
//	draw_frame(win->left_x+1, win->right_x-1, win->top_y + 11 + y*8, win->top_y + 18 + y*8);
}

// Moves the cursor position to the next line (do not draw anything)
void gui_next_line(Window *win) {
	win->cursor_x = win->left_x + 2;
	win->cursor_y += 8;

	if (win->text_end % 80 > 0) {
		uint bytes_to_end = 80 - (win->text_end % 80);
		memset(win->text + win->text_end, 0, bytes_to_end);
		win->text_end += bytes_to_end;
	}

	if (win->cursor_y + 8 > win->bottom_y - 2) gui_scroll(win);
}

void gui_set_cursor(Window *win) {
	if (window_focus != win) return;

	if (win->cursor_x + 8 > win->right_x - 2) gui_next_line(win);

	if (!strncmp(win->title, "Edit", 4))
		draw_edit_cursor(win->cursor_x, win->cursor_y);
	else
		draw_char(0, win->cursor_x, win->cursor_y);
}

void gui_remove_cursor(Window *win) {
	if (strncmp(win->title, "Edit", 4))
		draw_char(' ', win->cursor_x, win->cursor_y);
}

void gui_draw_window_header(Window *win, int focus) {
	draw_box(win->left_x+1, win->right_x-1, win->top_y+1, win->top_y+8);

	if (focus) {
		for (int j=win->top_y + 3; j<win->top_y + 9; j += 3) {
			for (int i=win->left_x + 3; i<win->right_x - 1; i+=3) {
				draw_pixel(i, j);
			}
		}
	}

	int len = strlen(win->title) * 8;
	draw_string(win->title, win->left_x + 1 + (win->right_x - win->left_x - len) / 2, win->top_y + 1);
}

void gui_set_focus(Window *win, Window *win_old) {
	window_focus = win;

	rect to_redraw;
	rect win1 = { .left_x = win->left_x, .right_x = win->right_x, .top_y = win->top_y, .bottom_y = win->bottom_y };
	rect win2 = { .left_x = win_old->left_x, .right_x = win_old->right_x, .top_y = win_old->top_y, .bottom_y = win_old->bottom_y };
	int nb_rects_to_redraw = intersection(&win1, &win2, &to_redraw);
	if (nb_rects_to_redraw > 0) {
//		gui_redraw_frame(win, to_redraw.left_x, to_redraw.right_x, to_redraw.top_y, to_redraw.bottom_y);
		gui_redraw(win, to_redraw.left_x, to_redraw.right_x, to_redraw.top_y, to_redraw.bottom_y);
	}

	gui_draw_window_header(win, 1);

	gui_set_cursor(win);
}

void gui_remove_focus(Window *win) {
	gui_draw_window_header(win, 0);
	gui_remove_cursor(win);
}

void gui_init(Window *win, const char *title) {
	int i;

	win->title = title;
	if (win->text == 0) win->text = (char*)kmalloc(4800);
	memset(win->text, 0, 4800);
	win->text_end = 0;

	// Draws the top and bottom lines of the window
	for (i=win->left_x; i<win->right_x; i++) {
		draw_pixel(i, win->top_y);
		draw_pixel(i, win->bottom_y);
		draw_pixel(i, win->top_y + 9);
	}

	gui_draw_window_header(win, (window_focus == win));

	// Draws the left and right lines of the window
	for (i=win->top_y; i<=win->bottom_y; i++) {
		draw_pixel(win->left_x, i);
		draw_pixel(win->right_x, i);
	}

	win->action->cls(win);

    win->action->puts(win, "Welcome to CHAOS (CHeers, Another Operating System) !");
    win->action->putcr(win);

    gui_save_mouse_buffer();
}

void gui_cls(Window *win) {
	draw_box(win->left_x+1, win->right_x-1, win->top_y+10, win->bottom_y-1);
	win->cursor_x = win->left_x + 2;
	win->cursor_y = win->top_y + 11;
	win->text_end = 0;
	gui_set_cursor(win);
}

void gui_puts(Window *win, const char *msg) {
	uint substring_len;
	uint bytes_to_end;
	const char *msg_start = msg;
	uint len = strlen(msg);

	// While the line to print is larger than the number of characters left on the row
	// Cut the first part to fill the row and go to the next line
	while (win->cursor_x + len * 8 > win->right_x - 2) {
		substring_len = (win->right_x - 2 - win->cursor_x) / 8;
		draw_string_n(msg_start, win->cursor_x, win->cursor_y,  substring_len);
		memcpy(win->text + win->text_end, msg_start, substring_len);

		msg_start += substring_len;
		win->text_end += substring_len;
		len -= substring_len;

		// This call updates win->cursor_x and win->cursor_y
		gui_next_line(win);
	}

	draw_string(msg_start, win->cursor_x, win->cursor_y);
	memcpy(win->text + win->text_end, msg_start, len);

	win->text_end += len;
	win->cursor_x += len*8;

	gui_set_cursor(win);
}

void gui_putc(Window *win, char c) {
	draw_char(c, win->cursor_x, win->cursor_y);

	win->cursor_x += 8;
	win->text[win->text_end++] = c;
	gui_set_cursor(win);
}

void gui_puti(Window *win, uint nb) {
	unsigned char *addr = (unsigned char *)&nb;
	char str_[11];
	char *str = (char*)&str_;
	str[0] = '0';
	str[1] = 'x';
	str[10] = 0;

	char *loc = str+1;

	char key[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	addr += 3;

	for (int i=0; i<4; i++) {
		unsigned char b = *addr--;
		int u = ((b & 0xf0) >> 4);
		int l = (b & 0x0f);
		*++loc = key[u];
		*++loc = key[l];
	}

	win->action->puts(win, str);
}

void gui_putnb(Window *win, int nb) {
	if (nb < 0) {
		win->action->putc(win, '-');
		nb = -nb;
	}
	uint nb_ref = 1000000000;
	uint leading_zero = 1;
	uint digit;

	for (int i=0; i<=9; i++) {
		if (nb >= nb_ref) {
			digit = nb / nb_ref;
			win->action->putc(win, '0' + digit);
			nb -= nb_ref * digit;

			leading_zero = 0;
		} else {
			if (!leading_zero) win->action->putc(win, '0');
		}
		nb_ref /= 10;
	}

	if (leading_zero) win->action->putc(win, '0');
}

void gui_putnb_right(Window *win, int nb) {
	char number[12];
	uint negative = 0;
	number[11] = 0;
	number[0] = ' ';

	if (nb < 0) {
		negative = 1;
		nb = -nb;
	}
	uint nb_ref = 1000000000;
	uint leading_zero = 1;
	uint digit;

	for (int i=0; i<=9; i++) {
		if (nb >= nb_ref) {
			digit = nb / nb_ref;
			number[i+1] = '0' + digit;
			nb -= nb_ref * digit;

			if (!leading_zero && negative) number[i] = '-';
			leading_zero = 0;
		} else {
			if (!leading_zero) number[i+1] = '0';
			else number[i+1] = ' ';
		}
		nb_ref /= 10;
	}

	if (leading_zero) number[10] = '0';

	gui_puts(win, number);
}

void gui_backspace(Window *win) {
	gui_remove_cursor(win);
	win->cursor_x -= 8;
	win->text_end--;
	gui_set_cursor(win);
}

void gui_putcr(Window *win) {
	gui_remove_cursor(win);
	gui_next_line(win);
	gui_set_cursor(win);
}

// Redraws the window frame and header - but only the part inside the box
void gui_redraw_frame(Window *win, uint left_x, uint right_x, uint top_y, uint bottom_y) {
	int i;

	// Draws the top and bottom lines of the window
	for (i=umax(win->left_x, left_x); i<umin(win->right_x, right_x+1); i++) {
		if (win->top_y >= top_y && win->top_y <= bottom_y) draw_pixel(i, win->top_y);
		if (win->bottom_y >= top_y && win->bottom_y <= bottom_y) draw_pixel(i, win->bottom_y);
		if (win->top_y+9 >= top_y && win->top_y+9 <= bottom_y) draw_pixel(i, win->top_y + 9);
	}

	if (window_focus == win) {
		for (int j=umax(win->top_y + 3, top_y); j<umin(win->top_y + 9, top_y+1); j += 3) {
			for (int i=umax(win->left_x + 3, left_x); i<umin(win->right_x - 1, right_x); i+=3) {
				draw_pixel(i, j);
			}
		}
	}

	int len = strlen(win->title) * 8;
	draw_string_inside_frame(win->title,
							 win->left_x + 1 + (win->right_x - win->left_x - len) / 2,
							 win->top_y + 1,
							 left_x, right_x, top_y, bottom_y);

	// Draws the left and right lines of the window
	for (i=umax(win->top_y, top_y); i<=umin(win->bottom_y, bottom_y+1); i++) {
		if (win->left_x >= left_x && win->left_x <= right_x) draw_pixel(win->left_x, i);
		if (win->right_x >= left_x && win->right_x <= right_x) draw_pixel(win->right_x, i);
	}	
}

void gui_redraw(Window *win, uint left_x, uint right_x, uint top_y, uint bottom_y) {
	draw_box(left_x, right_x, top_y, bottom_y);
	gui_redraw_frame(win, left_x, right_x, top_y, bottom_y);

	int max_text_offset_x = (win->right_x - win->left_x) / 8;
	int max_text_offset_y = (win->bottom_y - win->top_y) / 8;

	int left_text_offset = min(max(((int)left_x - (int)win->left_x) / 8, 0), max_text_offset_x);
	int right_text_offset = min(max(((int)right_x - (int)win->left_x) / 8 + 1, 0), max_text_offset_x);
	int top_text_offset = min(max(((int)top_y - (int)win->top_y) / 8, 0), max_text_offset_y);
	int bottom_text_offset = min(max(((int)bottom_y - (int)win->top_y) / 8 + 1, 0), max_text_offset_y);

	for (int j=top_text_offset; j<=bottom_text_offset; j++) {
		for (int i=left_text_offset; i<=right_text_offset; i++) {
			if (i+j*80 > win->text_end) break;

			const unsigned char c = win->text[i + j*80];
			if (c != 0) draw_char_inside_frame(c,
												 2 + i*8 + win->left_x, j*8 + win->top_y + 11,
												 left_x, right_x, top_y, bottom_y);
		}

	}
}

void gui_edit_cursor(Window *win, uint x, uint y) {
	win->cursor_x = win->left_x + 2 + x*8;
	win->cursor_y = win->top_y + 11 + y*8;
	draw_edit_cursor(win->cursor_x, win->cursor_y);
}

uint gui_max_x_chars(Window *win) {
	return (win->right_x - win->left_x - 2) / 8;
}

uint gui_max_y_chars(Window *win) {
	return (win->bottom_y - win->top_y - 2) / 8;
}

void gui_print(Window *win, const char *text, uint x, uint y) {
	draw_string(text, win->left_x + 2 + x*8, win->top_y + 11 + y*8);
}

void gui_printc(Window *win, char c, uint x, uint y) {
	draw_font(c, win->left_x + 2 + x*8, win->top_y + 11 + y*8);
}

// Definition of the various windowing primitives in the GUI environment
struct WindowAction gui_window_action = {
	.init = &gui_init,
	.cls = &gui_cls,
	.puts = &gui_puts,
	.putc = &gui_putc,
	.puti = &gui_puti,
	.putnb = &gui_putnb,
	.putnb_right = &gui_putnb_right,
	.backspace = &gui_backspace,
	.putcr = &gui_putcr,
	.set_cursor = &gui_set_cursor,
	.cursor = &gui_edit_cursor,
	.max_x_chars = &gui_max_x_chars,
	.max_y_chars = &gui_max_y_chars,
	.print = &gui_print,
	.printc = &gui_printc,
	.set_focus = &gui_set_focus,
	.remove_focus = &gui_remove_focus,
	.redraw = &gui_redraw,
	.scroll_down = &gui_scroll_down
};

// We define two windows
Window gui_win1 = {
	.left_x = 150,
	.right_x = 600,
	.top_y = 10,
	.bottom_y = 220,
	.action = &gui_window_action
};

Window gui_win2 = {
	.left_x = 20,
	.right_x = 630,
	.top_y = 230,
	.bottom_y = 470,
	.action = &gui_window_action
};

// When the user clicks on the mouse, the focus window may change
Window *gui_handle_mouse_click(uint mouse_x, uint mouse_y) {
	Window *new_window_focus = window_focus;

	do {
		if (mouse_x >= new_window_focus->left_x &&
			mouse_x <= new_window_focus->right_x &&
			mouse_y >= new_window_focus->top_y &&
			mouse_y <= new_window_focus->bottom_y) break;

		new_window_focus = new_window_focus->next;
	} while (new_window_focus != window_focus);

	if (new_window_focus == window_focus) return window_focus;

	window_focus->action->remove_focus(window_focus);
	new_window_focus->action->set_focus(new_window_focus, window_focus);

	return window_focus;
}
