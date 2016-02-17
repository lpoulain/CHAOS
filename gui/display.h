#ifndef __DISPLAY_H
#define __DISPLAY_H

#define WHITE_ON_BLACK 0x0f
#define YELLOW_ON_BLACK 0x0e

#define BUFFER_SIZE 128

#define TEXT_MODE	0
#define VGA_MODE	1

#define WINDOW(win, act, ...) win->action->(win, ##__VA_ARGS__)

struct WindowAction;
typedef struct process_t Process;
typedef struct window_t Window;

typedef struct window_t {
	uint8 type;

	// Text fields
	unsigned char *start_address;
	unsigned char *end_address;
	unsigned char *cursor_address;
	char text_color;

	// GUI fields
	uint left_x;
	uint right_x;
	uint top_y;
	uint bottom_y;
	uint height;
	uint cursor_x;
	uint cursor_y;
	char *text;
	uint text_end;

	// Common fields
	unsigned char buffer[BUFFER_SIZE];
	int buffer_end;	
	struct WindowAction *action;
	const char *title;
	Process *ps;
	Window *next;
} Window;

extern Window *window_focus;
extern Window *window_debug;

// This class is used for polymorphism
// The action can either be for a text of a GUI window
struct WindowAction {
	void (*init) (Window *, const char *);
	void (*cls) (Window *);
	void (*puts) (Window *, const char *);
	void (*putc) (Window *, char);
	void (*puti) (Window *, uint);
	void (*putnb) (Window *, int);
	void (*putnb_right) (Window *, int);
	void (*backspace) (Window *);
	void (*putcr) (Window *);
	void (*set_cursor) (Window *);
	void (*set_focus) (Window *, Window *);
	void (*remove_focus) (Window *);
	void (*redraw) (Window *, uint, uint, uint, uint);
};

uint display_mode();
Process *get_process_focus();
void print(const char *, int, int, char);
void print_hex(char b, int row, int col);
void print_hex2(char b, int row, int col);
void print_c(char c, int row, int col);
void puts(Window *win, const char *);
void putc(Window *win, char);
void puti(Window *win, uint);
void putnb(Window *win, int nb);
void cls(Window *win);
void putcr(Window *win);
void backspace(Window *win);
void set_cursor(Window *win);
void print_ptr(void *ptr, int row, int col);
void print_int(int n, int row, int col);
void debug_i(char *msg, uint nb);
void debug(char *msg);
void switch_window_focus();

void mouse_move_text(int delta_x, int delta_y);

void init_window(Window *win, Process *ps);
Window *set_window_debug(Window *new);
void printf_win(Window *, const char *format, ...);

extern void (*dump_mem)(void *, int, int);

#endif
