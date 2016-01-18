#ifndef __DISPLAY_H
#define __DISPLAY_H

#define WHITE_ON_BLACK 0x0f
#define YELLOW_ON_BLACK 0x0e

#define BUFFER_SIZE 128

// A display is a text window which can receive user input
/*
typedef struct {
	unsigned char *start_address;
	unsigned char *end_address;
	unsigned char *cursor_address;
	char text_color;
	unsigned char buffer[BUFFER_SIZE];
	int buffer_end;
} display;
*/
#define WINDOW_GUI	1
#define WINDOW_TEXT	2

#define WINDOW(win, act, ...) win->action->(win, ##__VA_ARGS__)

struct window_action;

typedef struct {
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
	char text[80*60];

	// Common fields
	unsigned char buffer[BUFFER_SIZE];
	int buffer_end;	
	struct window_action *action;
	const char *title;
} window;

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
	void (*set_focus) (window *);
	void (*remove_focus) (window *);
};

void init_screen();
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
void dump_mem(void *ptr, int nb_bytes, int row);
void debug_i(char *msg, uint nb);
void debug(char *msg);

void mouse_move_text(int delta_x, int delta_y);

void text_init_display(window *win, int row_start, int row_end, int color);

#endif
