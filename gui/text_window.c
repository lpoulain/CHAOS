#include "libc.h"
#include "display.h"
#include "display_text.h"

// Prints a carriage return on the window
void text_putcr(window *win) {
	if (win->cursor_address == win->end_address) {
		text_scroll(win);
	}

	uint offset = (win->cursor_address - win->start_address) % 160;
	win->cursor_address += 160 - offset;
}

// Prints a character on the window
void text_putc(window *win, char c) {
	if (c == '\n') {
		text_putcr(win);
	} else {
		if (win->cursor_address >= win->end_address) text_scroll(win);

		*win->cursor_address++ = c;
		*win->cursor_address++ = win->text_color;
	}
}

// Prints a string on the window
void text_puts(window *win, const char *text) {
	while (*text) text_putc(win, *text++);
}

// Prints an integer (in hex format) on the window
void text_puti(window *win, uint i) {
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

	text_puts(win, loc);
}

// Prints a number on the window
void text_putnb(window *win, int nb) {
	if (nb < 0) {
		text_putc(win, '-');
		nb = -nb;
	}
	uint nb_ref = 1000000000;
	uint leading_zero = 1;
	uint digit;

	for (int i=0; i<=9; i++) {
		if (nb >= nb_ref) {
			digit = nb / nb_ref;
			text_putc(win, '0' + digit);
			nb -= nb_ref * digit;

			leading_zero = 0;
		} else {
			if (!leading_zero) text_putc(win, '0');
		}
		nb_ref /= 10;
	}
}

// When the user presses backspace
void text_backspace(window *win) {
	// We don't want to go before the video buffer
	if (win->cursor_address == win->start_address) return;
	win->cursor_address -= 2;
	*win->cursor_address = ' ';
}

void text_set_focus(window *win, window *win_unused) {
	window_focus = win;
	text_set_cursor(win);
}

void text_remove_focus(window *win) { }

void text_init(window *win, const char *title) {
	win->start_address = (unsigned char*)VIDEO_ADDRESS + win->top_y * 160;
	win->end_address = (unsigned char*)VIDEO_ADDRESS + (win->bottom_y + 1) * 160;
	win->cursor_address = win->start_address;
	win->buffer_end = 0;
}

// Definition of the various windowing primitives in the text environment
struct window_action text_window_action = {
	.init = &text_init,
	.cls = &text_cls,
	.puts = &text_puts,
	.putc = &text_putc,
	.puti = &text_puti,
	.putnb = &text_putnb,
	.backspace = &text_backspace,
	.putcr = &text_putcr,
	.set_cursor = &text_set_cursor,
	.set_focus = &text_set_focus,
	.remove_focus = &text_remove_focus
};

// We define two windows
window text_win1 = {
	.top_y = 1,
	.bottom_y = 12,
	.text_color = 0x0e,
	.action = &text_window_action
};

window text_win2 = {
	.top_y = 12,
	.bottom_y = 24,
	.text_color = 0x0f,
	.action = &text_window_action
};
