#include "libc.h"
#include "shell.h"
#include "process.h"
#include "parser.h"
#include "floppy.h"

extern process *current_process;
extern process *process_focus;
extern void stack_dump();

typedef struct {
	char entry[100][BUFFER_SIZE];
	uint entry_idx;
} cmd_hist;

void prompt(display *disp) {
	puts(disp, "CHAOS> ");
}

void countdown(display *disp) {
	int counter = 10;
	char *nb = "987654321";
	for (int k=0; k<10; k++) {
		for (int i=0; i<1000; i++) {
			for (int j=0; j<10000; j++) {
			}
		}
		putc(disp, nb[k]);
	}
	putcr(disp);
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

void process_command(display *disp) {

	// Clear screen
	if (!strcmp(disp->buffer, "cls")) {
		cls(disp);
		return;
	}

	// Read from the disk (work in progress)
	if (!strcmp(disp->buffer, "f")) {
		floppy_test_read();
		return;
	}

	if (!strcmp(disp->buffer, "help")) {
		puts(disp, "Commands:"); putcr(disp);
		puts(disp, "- cls: clear screen"); putcr(disp);
		puts(disp, "- help: this help"); putcr(disp);
		puts(disp, "- stack: prints the stack trace"); putcr(disp);
		puts(disp, "- countdown: pegs the CPU for a few seconds (to test multitasking)"); putcr(disp);
		puts(disp, "- mem: main memory pointers"); putcr(disp);
		puts(disp, "- mem [hex address]: memory dump (to test paging)"); putcr(disp);
		puts(disp, "- [integer formula]: e.g. 5 + 4*(3 - 1)");
		putcr(disp);
		return;
	}

	// Run a countdown
	if (!strcmp(disp->buffer, "countdown")) {
		countdown(disp);
		return;
	}

	// Display the current stack trace
	if (!strcmp(disp->buffer, "stack")) {
		stack_dump();
		return;
	}

	// Prints the main memory pointers
	if (!strcmp(disp->buffer, "mem")) {
	    debug_i("code:       ", (uint)&code);
	    debug_i("r/o data:   ", (uint)&rodata);
	    debug_i("data:       ", (uint)&data);
    	debug_i("bss:        ", (uint)&bss);
    	debug_i("debug_info: ", (uint)&debug_info);
    	debug_i("end:        ", (uint)&end);
		return;
	}

	// Memory dump
	if (!strncmp(disp->buffer, "mem ", 4)) {
		uint bufferpos = 4;
		if (!strncmp(disp->buffer + 4, "0x", 2)) bufferpos += 2;

		uint val = 268435456;
		uint address = 0;
		char c;

		for (int i=bufferpos; i<bufferpos+8; i++) {
			c = disp->buffer[i];
			if (c == 0) break;
			if (c >= 'A' && c <= 'F') c += 32;
			if (c >= 'a' && c <= 'f') address += val * (10 + (c - 'a'));
			else if (c >= '0' && c <= '9') address += val * (c - '0');
			else {
				puts(disp, "Invalid character in address: ");
				putc(disp, c);
				putcr(disp);
				return;
			}

			val /= 16;
		}

		dump_mem_addr = (uint)address;
		dump_mem((void*)address, 160, 13);
		return;
	}

	// Parsing arithmetic operations
	int res = parse(disp->buffer);
	if (res <= 0) {
		for (int i=0; i<7-res; i++) putc(disp, ' ');
		putc(disp, '^');
		putcr(disp);
		puts(disp, "Syntax error");
		putcr(disp);
		return;
	}

	int value;
	current_process->error[0] = 0;		// reset error
	res = is_math_formula(0, nb_tokens, &value);
	if (res > 0) {
		putnb(disp, value);
		putcr(disp);
		return;
	} else if (res < 0) {
		for (int i=0; i<7-res; i++) putc(disp, ' ');
		putc(disp, '^');
		putcr(disp);
		if (current_process->error[0] != 0) {
			puts(disp, current_process->error);
			putcr(disp);
		}
		return;
	}

	// If everything else fails
	puts(disp, "Invalid command");
	putcr(disp);
}

void process_char(unsigned char c, cmd_hist *commands) {
	display *disp = &get_process_focus()->disp;

	// If no keyboard key is captured do nothing
	if (c == 0) return;

	// Up or down arrow: retreive the current command
	if (c == 1 || c == 2) {
		uint idx = commands->entry_idx;
		idx += (c - 1) * 2 - 1;
		if (idx < 0) idx = 99;
		if (idx >= 100) idx = 0;

		if (commands->entry[idx][0] == 0 && disp->buffer_end == 0) return;

		for (int i=0; i<disp->buffer_end; i++) backspace(disp);
		strcpy(disp->buffer, commands->entry[idx]);
		disp->buffer_end = strlen(disp->buffer);
		puts(disp, disp->buffer);
		set_cursor(disp);
		commands->entry_idx = idx;
		return;
	}

	// Left or right arrow: change the mem dump address region
	if (c == 3 || c == 4) {
		dump_mem_addr += ((c - 3) * 2 - 1) * 176;
		if (dump_mem_addr < 0) dump_mem_addr = 0;
		dump_mem((void*)dump_mem_addr, 160, 13);
		return;
	}

	// The user pressed Enter. Processing the command buffer
	if (c == '\n') {
		disp->buffer[disp->buffer_end] = 0;
		putcr(disp);
		if (disp->buffer_end != 0) {
	
			uint idx = commands->entry_idx;
			strcpy(commands->entry[idx], disp->buffer);
			idx++;
			if (idx >= 100) idx = 0;
			commands->entry_idx = idx;

			process_command(disp);
		}
		prompt(disp);
		set_cursor(disp);
		disp->buffer_end = 0;
		return;
	}

	// The user pressed backspace
	if (c == '\b') {
		if (disp->buffer_end == 0) return;
		backspace(disp);
		set_cursor(disp);
		disp->buffer[disp->buffer_end--] = 0;
		return;
	}

	// If the command buffer is full, stop
	if (disp->buffer_end >= BUFFER_SIZE - 1) return;

	// Otherwise fill the buffer
	disp->buffer[disp->buffer_end++] = c;

	// And print the character on the screen
	putc(disp, c);
	set_cursor(disp);

	print_hex(c, 0, 75);
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
	display *disp = &(current_process->disp);

	cmd_hist commands;
	memset(&commands, 0, sizeof(cmd_hist));

	prompt(disp);
	if (process_focus == current_process) set_cursor(disp);
	disp->buffer_end = 0;

//	countdown(disp);
//	prompt(disp);

	for (;;) {
		unsigned char c = poll();
		process_char(c, &commands);
	}
}
