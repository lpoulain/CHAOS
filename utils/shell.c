#include "libc.h"
#include "shell.h"
#include "process.h"
#include "parser.h"
#include "floppy.h"
#include "vga.h"
#include "gui_mouse.h"

extern process *current_process;
extern process *process_focus;
extern void stack_dump();
extern void read_sectors();

typedef struct {
	char entry[100][BUFFER_SIZE];
	uint entry_idx;
} cmd_hist;

void prompt(window *win) {
	win->action->puts(win, "CHAOS> ");
}

void countdown(window *win) {
	int counter = 10;
	char *nb = "9876543210";
	for (int k=0; k<10; k++) {
		for (int i=0; i<10000; i++) {
			for (int j=0; j<10000; j++) {
			}
		}
		win->action->putc(win, nb[k]);
	}
	win->action->putcr(win);
}

extern token tokens[10];
extern int nb_tokens;
extern uint code;
extern uint data;
extern uint rodata;
extern uint bss;
extern uint debug_info;
//extern uint debug_line;
extern uint end;
uint dump_mem_addr = 0;

void process_command(window *win) {

	// Clear screen
	if (!strcmp(win->buffer, "cls")) {
		win->action->cls(win);
		return;
	}

	// Read from the disk (work in progress)
	if (!strcmp(win->buffer, "f")) {
		floppy_test_read();
		return;
	}

	if (!strcmp(win->buffer, "help")) {
		win->action->puts(win, "Commands:"); win->action->putcr(win);
		win->action->puts(win, "- cls: clear screen"); win->action->putcr(win);
		win->action->puts(win, "- help: this help"); win->action->putcr(win);
		win->action->puts(win, "- stack: prints the stack trace"); win->action->putcr(win);
		win->action->puts(win, "- countdown: pegs the CPU for a few seconds (to test multitasking)"); win->action->putcr(win);
		win->action->puts(win, "- mem: main memory pointers"); win->action->putcr(win);
		win->action->puts(win, "- mem [hex address]: memory dump (to test paging)"); win->action->putcr(win);
		win->action->puts(win, "- [integer formula]: e.g. 5 + 4*(3 - 1)");
		win->action->putcr(win);
		return;
	}

	// Run a countdown
	if (!strcmp(win->buffer, "countdown")) {
		countdown(win);
		return;
	}

	// Display the current stack trace
	if (!strcmp(win->buffer, "stack")) {
		stack_dump();
		return;
	}

	// Prints the main memory pointers
	if (!strcmp(win->buffer, "mem")) {
	    debug_i("code:       ", (uint)&code);
	    debug_i("r/o data:   ", (uint)&rodata);
	    debug_i("data:       ", (uint)&data);
    	debug_i("bss:        ", (uint)&bss);
    	debug_i("debug_info: ", (uint)&debug_info);
    	debug_i("end:        ", (uint)&end);
		return;
	}

	// Memory dump
	if (!strncmp(win->buffer, "mem ", 4)) {
		uint bufferpos = 4;
		if (!strncmp(win->buffer + 4, "0x", 2)) bufferpos += 2;

		uint val = 268435456;
		uint address = 0;
		char c;

		for (int i=bufferpos; i<bufferpos+8; i++) {
			c = win->buffer[i];
			if (c == 0) break;
			if (c >= 'A' && c <= 'F') c += 32;
			if (c >= 'a' && c <= 'f') address += val * (10 + (c - 'a'));
			else if (c >= '0' && c <= '9') address += val * (c - '0');
			else {
				win->action->puts(win, "Invalid character in address: ");
				win->action->putc(win, c);
				win->action->putcr(win);
				return;
			}

			val /= 16;
		}

		dump_mem_addr = (uint)address;
		dump_mem((void*)address, 160, 48);
		return;
	}

	// Parsing arithmetic operations
	int res = parse(win->buffer);
	if (res <= 0) {
		for (int i=0; i<7-res; i++) putc(win, ' ');
		win->action->putc(win, '^');
		win->action->putcr(win);
		win->action->puts(win, "Syntax error");
		win->action->putcr(win);
		return;
	}

	// Draw box
	if (nb_tokens == 5 &&
		tokens[0].code == PARSE_WORD && !strcmp(tokens[0].value, "box") &&
		tokens[1].code == PARSE_NUMBER && tokens[2].code == PARSE_NUMBER && tokens[3].code == PARSE_NUMBER && tokens[4].code == PARSE_NUMBER) {

		draw_box((uint)tokens[1].value, (uint)tokens[2].value, (uint)tokens[3].value, (uint)tokens[4].value);

		win->action->putcr(win);
		return;
	}

	// Draw frame
	if (nb_tokens == 5 &&
		tokens[0].code == PARSE_WORD && !strcmp(tokens[0].value, "frame") &&
		tokens[1].code == PARSE_NUMBER && tokens[2].code == PARSE_NUMBER && tokens[3].code == PARSE_NUMBER && tokens[4].code == PARSE_NUMBER) {

		draw_background((uint)tokens[1].value, (uint)tokens[2].value, (uint)tokens[3].value, (uint)tokens[4].value);
		draw_frame((uint)tokens[1].value, (uint)tokens[2].value, (uint)tokens[3].value, (uint)tokens[4].value);

		win->action->putcr(win);
		return;
	}

	int value;
	current_process->error[0] = 0;		// reset error
	res = is_math_formula(0, nb_tokens, &value);
	if (res > 0) {
		win->action->putnb(win, value);
		win->action->putcr(win);
		return;
	} else if (res < 0) {
		for (int i=0; i<7-res; i++) win->action->putc(win, ' ');
		win->action->putc(win, '^');
		win->action->putcr(win);
		if (current_process->error[0] != 0) {
			win->action->puts(win, current_process->error);
			win->action->putcr(win);
		}
		return;
	}

	// If everything else fails
	win->action->puts(win, "Invalid command");
	win->action->putcr(win);
}

void process_char(unsigned char c, cmd_hist *commands) {
	window *win = &get_process_focus()->win;

	// If no keyboard key is captured do nothing
	if (c == 0) return;

	// Up or down arrow: retreive the current command
	if (c == 1 || c == 2) {
		uint idx = commands->entry_idx;
		idx += (c - 1) * 2 - 1;
		if (idx < 0) idx = 99;
		if (idx >= 100) idx = 0;

		if (commands->entry[idx][0] == 0 && win->buffer_end == 0) return;

		for (int i=0; i<win->buffer_end; i++) win->action->backspace(win);
		strcpy(win->buffer, commands->entry[idx]);
		win->buffer_end = strlen(win->buffer);
		win->action->puts(win, win->buffer);
		win->action->set_cursor(win);
		commands->entry_idx = idx;
		return;
	}

	// Left or right arrow: change the mem dump address region
	if (c == 3 || c == 4) {
		dump_mem_addr += ((c - 3) * 2 - 1) * 176;
		if (dump_mem_addr < 0) dump_mem_addr = 0;
		dump_mem((void*)dump_mem_addr, 160, 48);
		return;
	}

	// The user pressed Enter. Processing the command buffer
	if (c == '\n') {
		win->buffer[win->buffer_end] = 0;
		win->action->putcr(win);
		if (win->buffer_end != 0) {
	
			uint idx = commands->entry_idx;
			strcpy(commands->entry[idx], win->buffer);
			idx++;
			if (idx >= 100) idx = 0;
			commands->entry_idx = idx;

			process_command(win);
		}
		prompt(win);
		win->action->set_cursor(win);
		win->buffer_end = 0;
		return;
	}

	// The user pressed backspace
	if (c == '\b') {
		if (win->buffer_end == 0) return;
		win->action->backspace(win);
		win->action->set_cursor(win);
		win->buffer[win->buffer_end--] = 0;
		return;
	}

	// If the command buffer is full, stop
	if (win->buffer_end >= BUFFER_SIZE - 1) return;

	// Otherwise fill the buffer
	win->buffer[win->buffer_end++] = c;

	// And print the character on the screen
	win->action->putc(win, c);
	win->action->set_cursor(win);

//	print_hex(c, 0, 75);
}

extern int new_process;
extern int polling;
extern process *current_process;
extern void context_switch();

unsigned char poll() {
	current_process->flags |= PROCESS_POLLING;
	char c;

	for (;;) {
		if (current_process->buffer) {
			c = current_process->buffer;
			current_process->buffer = 0;
			return c;
		}
	}

	return current_process->buffer;
}

void shell() {
	window *win = &(current_process->win);
	win->action->init(win, "Shell");

	cmd_hist commands;
	memset(&commands, 0, sizeof(cmd_hist));

	prompt(win);
	if (process_focus == current_process) win->action->set_cursor(win);
	win->buffer_end = 0;

	for (;;) {
		unsigned char c = poll();
		gui_mouse_hide();
		process_char(c, &commands);
		gui_mouse_show();
	}
}
