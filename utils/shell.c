#include "libc.h"
#include "shell.h"
#include "process.h"
#include "parser.h"

extern process *current_process;
extern void stack_dump();

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

void process_command(display *disp) {
	int res = parse(disp->buffer);
	if (res <= 0) {
		puts(disp, "Syntax error (character ");
		putnb(disp, 1-res);
		puts(disp, ")");
		putcr(disp);
	}
/*
	for (int i=0; i<nb_tokens; i++) {
		puts(disp, "Token: ");
		puti(disp, tokens[i].code);
		puts(disp, " ");
		switch(tokens[i].code) {
			case PARSE_WORD:
				puts(disp, tokens[i].value);
				break;
			case PARSE_NUMBER:
				puti(disp, (uint)tokens[i].value);
				break;
		}
		putcr(disp);
	}*/

	if (nb_tokens == 3 && tokens[0].code == PARSE_NUMBER && tokens[1].code == PARSE_PLUS && tokens[2].code == PARSE_NUMBER) {
		puts(disp, "=> ");
		putnb(disp, (uint)tokens[0].value + (uint)tokens[2].value);
		putcr(disp);
		return;
	}

	if (nb_tokens == 3 && tokens[0].code == PARSE_NUMBER && tokens[1].code == PARSE_MINUS && tokens[2].code == PARSE_NUMBER) {
		puts(disp, "=> ");
		putnb(disp, (uint)tokens[0].value - (uint)tokens[2].value);
		putcr(disp);
		return;
	}

	if (nb_tokens == 3 && tokens[0].code == PARSE_NUMBER && tokens[1].code == PARSE_MULT && tokens[2].code == PARSE_NUMBER) {
		puts(disp, "=> ");
		putnb(disp, (uint)tokens[0].value * (uint)tokens[2].value);
		putcr(disp);
		return;
	}

	if (nb_tokens == 3 && tokens[0].code == PARSE_NUMBER && tokens[1].code == PARSE_DIV && tokens[2].code == PARSE_NUMBER) {
		if ((uint)tokens[2].value == 0) {
			puts(disp, "Error: division by zero");
			putcr(disp);
			return;
		}
		puts(disp, "=> ");
		putnb(disp, (uint)tokens[0].value / (uint)tokens[2].value);
		putcr(disp);
		return;
	}

	if (!strcmp(disp->buffer, "cls")) {
		cls(disp);
		return;
	}

	if (!strcmp(disp->buffer, "help")) {
		puts(disp, "Commands:"); putcr(disp);
		puts(disp, "- cls: clear screen"); putcr(disp);
		puts(disp, "- help: this help"); putcr(disp);
		puts(disp, "- stack: prints the stack trace"); putcr(disp);
		puts(disp, "- countdown: pegs the CPU for a few seconds (to test multitasking)"); putcr(disp);
		puts(disp, "- mem [hex address]: memory dump (to test paging)"); putcr(disp);
		puts(disp, "- [number] [operation] [number]: basic arithmetic operations");
		putcr(disp);
		return;
	}

	if (!strcmp(disp->buffer, "countdown")) {
		countdown(disp);
		return;
	}

	if (!strcmp(disp->buffer, "stack")) {
		stack_dump();
		return;
	}

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

		dump_mem((void*)address, 160, 13);
		return;
	}

	puts(disp, "Invalid command");
	putcr(disp);
}

void process_char(unsigned char c) {
	display *disp = &get_process_focus()->disp;

	// If a shift key is pressed
	if (c == 0) return;

	// The user pressed Enter. Processing the command buffer
	if (c == '\n') {
		disp->buffer[disp->buffer_end] = 0;
		putcr(disp);
		if (disp->buffer_end != 0) {
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

	prompt(disp);
	disp->buffer_end = 0;

//	countdown(disp);
//	prompt(disp);

	for (;;) {
		unsigned char c = poll();
		process_char(c);
	}
}
