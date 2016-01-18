#include "libc.h"
#include "display.h"
#include "process.h"
#include "vga.h"
#include "gui_mouse.h"

extern process *process_focus;
extern process processes[3];

///////////////////////////////////////////////////////////////
// GUI Window functions
///////////////////////////////////////////////////////////////

// We've reached the bottom of the window, we need to scroll everything up one line
void gui_scroll(window *win) {
	copy_box(win->left_x+1, win->right_x-1, win->top_y + 10, win->cursor_y - 8, 8);
	draw_box(win->left_x+1, win->right_x-1, win->cursor_y - 7, win->cursor_y);
	win->cursor_y -= 8;
}

// Moves the cursor position to the next line (do not draw anything)
void gui_next_line(window *win) {
	win->cursor_x = win->left_x + 2;
	win->cursor_y += 8;

	if (win->cursor_y + 8 > win->bottom_y - 2) gui_scroll(win);
}

void gui_set_cursor(window *win) {
	if (&process_focus->win != win) return;

	if (win->cursor_x + 8 > win->right_x - 2) gui_next_line(win);

	draw_font(0, win->cursor_x, win->cursor_y);
}

void gui_remove_cursor(window *win) {
	draw_font(' ', win->cursor_x, win->cursor_y);
}

void gui_draw_window_header(window *win, int focus) {
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

void gui_set_focus(window *win) {
	gui_draw_window_header(win, 1);
	gui_set_cursor(win);
}

void gui_remove_focus(window *win) {
	gui_draw_window_header(win, 0);
	gui_remove_cursor(win);
}

void gui_init(window *win, const char *title) {
	int i;

	win->title = title;

	// Draws the top and bottom lines of the window
	for (i=win->left_x; i<win->right_x; i++) {
		draw_pixel(i, win->top_y);
		draw_pixel(i, win->bottom_y);
		draw_pixel(i, win->top_y + 9);
	}

	gui_draw_window_header(win, (&process_focus->win == win));

	int len = strlen(title) * 8;
	draw_string(title, win->left_x + 1 + (win->right_x - win->left_x - len) / 2, win->top_y + 1);

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

void gui_cls(window *win) {
	draw_box(win->left_x+1, win->right_x-1, win->top_y+10, win->bottom_y-1);
	win->cursor_x = win->left_x + 2;
	win->cursor_y = win->top_y + 11;
	gui_set_cursor(win);
}

void gui_puts(window *win, const char *msg) {
	uint len = strlen(msg);
	uint substring_len;
	const char *msg_start = msg;

	while (win->cursor_x + len * 8 > win->right_x - 2) {
		substring_len = (win->right_x - 2 - win->cursor_x) / 8;
		draw_string_n(msg_start, win->cursor_x, win->cursor_y,  substring_len);
		msg_start += substring_len;
		len -= substring_len;

		gui_next_line(win);
	}

	draw_string(msg_start, win->cursor_x, win->cursor_y);

	win->cursor_x += len*8;
	gui_set_cursor(win);
}

void gui_putc(window *win, char c) {
	draw_font(c, win->cursor_x, win->cursor_y);

	win->cursor_x += 8;
	gui_set_cursor(win);
}

void gui_puti(window *win, uint nb) {
	unsigned char *addr = (unsigned char *)&nb;
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

	win->action->puts(win, loc);
}

void gui_putnb(window *win, int nb) {
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
}

void gui_backspace(window *win) {
	gui_remove_cursor(win);
	win->cursor_x -= 8;
	gui_set_cursor(win);
}

void gui_putcr(window *win) {
	gui_remove_cursor(win);
	gui_next_line(win);
	gui_set_cursor(win);
}

// Definition of the various windowing primitives in the GUI environment
struct window_action gui_window_action = {
	.init = &gui_init,
	.cls = &gui_cls,
	.puts = &gui_puts,
	.putc = &gui_putc,
	.puti = &gui_puti,
	.putnb = &gui_putnb,
	.backspace = &gui_backspace,
	.putcr = &gui_putcr,
	.set_cursor = &gui_set_cursor,
	.set_focus = &gui_set_focus,
	.remove_focus = &gui_remove_focus
};

// We define two windows
window win1 = {
	.left_x = 10,
	.right_x = 500,
	.top_y = 10,
	.bottom_y = 200,
	.action = &gui_window_action
};

window win2 = {
	.left_x = 20,
	.right_x = 630,
	.top_y = 230,
	.bottom_y = 470,
	.action = &gui_window_action
};

// When the user clicks on the mouse, the focus window may change
window *gui_handle_mouse_click(uint mouse_x, uint mouse_y) {
	for (int i=0; i<2; i++) {
		if (mouse_x >= processes[i].win.left_x &&
			mouse_x <= processes[i].win.right_x &&
			mouse_y >= processes[i].win.top_y &&
			mouse_y <= processes[i].win.bottom_y) {

			process_focus->win.action->remove_focus(&process_focus->win);
			process_focus = &processes[i];
			process_focus->win.action->set_focus(&process_focus->win);
			return &process_focus->win;
		}
	}

	return 0;
}
