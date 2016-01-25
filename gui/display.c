#include "libc.h"
#include "kernel.h"
#include "display.h"
#include "process.h"
#include "display_vga.h"
#include "display_text.h"
#include "gui_screen.h"
#include "gui_mouse.h"
#include "text_mouse.h"

uint vga;
window *window_focus;

// Tells whether we are in VGA or text
uint display_mode() {
	if (vga) return VGA_MODE;
	return TEXT_MODE;
}

// Initialize

process *get_process_focus() {
	return window_focus->ps;
}

void (*mouse_move)(int, int, uint);
void (*mouse_click)();
void (*mouse_unclick)();
void (*mouse_show)();
void (*mouse_hide)();
void (*dump_mem)(void *, int, int);

void init_window(window *win, process *ps) {
	win->ps = ps;
	ps->win = win;

	if (window_focus == 0) {
		window_focus = win;
		win->next = win;
	} else {
		window *tmp = window_focus->next;
		window_focus->next = win;
		win->next = tmp;
	}
}

// Changes the process which has the focus
void switch_window_focus() {
    if (window_focus->next == window_focus) return;

    gui_mouse_hide();
    window_focus->action->remove_focus(window_focus);
    window_focus->action->set_focus(window_focus->next, window_focus);
    gui_mouse_show();
}

void init_display(uint flags) {
	draw_box(0, 0, 100, 100);
	draw_ptr((void*)flags, 0, 0);

	text_print_int(flags, 0, 0);

	if (flags & 0x4) {
		init_vga();
		mouse_move = &gui_mouse_move;
		mouse_click = &gui_mouse_click;
		mouse_unclick = &gui_mouse_unclick;
		mouse_show = &gui_mouse_show;
		mouse_hide = &gui_mouse_hide;
		dump_mem = &gui_dump_mem;
		vga = 1;
	} else {
		init_text();
		mouse_move = &text_mouse_move;
		mouse_click = &text_mouse_click;
		mouse_unclick = &text_mouse_unclick;
		mouse_show = &text_mouse_show;
		mouse_hide = &text_mouse_hide;
		dump_mem = &text_dump_mem;
		vga = 0;
	}
}
