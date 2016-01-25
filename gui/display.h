#ifndef __DISPLAY_H
#define __DISPLAY_H

#define WHITE_ON_BLACK 0x0f
#define YELLOW_ON_BLACK 0x0e

#define BUFFER_SIZE 128

#define TEXT_MODE	0
#define VGA_MODE	1

#define WINDOW(win, act, ...) win->action->(win, ##__VA_ARGS__)

struct window_action;
typedef struct process_t process;
typedef struct window_t window;

typedef struct window_t {
	u8int type;

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
	struct window_action *action;
	const char *title;
	process *ps;
	window *next;
} window;

extern window *window_focus;

// This class is used for polymorphism
// The action can either be for a text of a GUI window
struct window_action {
	void (*init) (window *, const char *);
	void (*cls) (window *);
	void (*puts) (window *, const char *);
	void (*putc) (window *, char);
	void (*puti) (window *, uint);
	void (*putnb) (window *, int);
	void (*backspace) (window *);
	void (*putcr) (window *);
	void (*set_cursor) (window *);
	void (*set_focus) (window *, window *);
	void (*remove_focus) (window *);
	void (*redraw) (window *, uint, uint, uint, uint);
};

uint display_mode();
process *get_process_focus();
void print(const char *, int, int, char);
void print_hex(char b, int row, int col);
void print_hex2(char b, int row, int col);
void print_c(char c, int row, int col);
void puts(window *win, const char *);
void putc(window *win, char);
void puti(window *win, uint);
void putnb(window *win, int nb);
void cls(window *win);
void putcr(window *win);
void backspace(window *win);
void set_cursor(window *win);
void print_ptr(void *ptr, int row, int col);
void print_int(int n, int row, int col);
void debug_i(char *msg, uint nb);
void debug(char *msg);
void switch_window_focus();

void mouse_move_text(int delta_x, int delta_y);

void init_window(window *win, process *ps);

extern void (*dump_mem)(void *, int, int);

#endif
