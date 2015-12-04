#include "kernel.h"
#include "libc.h"
#include "keyboard.h"
#include "display.h"

#define BUFFER_SIZE 128

unsigned char buffer[BUFFER_SIZE];
int buffer_end;

void prompt() {
	puts("CHAOS> ");
}

void process_command() {
	if (!strcmp(buffer, "cls")) {
		cls();
		return;
	}

	if (!strcmp(buffer, "help")) {
		puts("Commands:"); putcr();
		puts("- cls: clear screen"); putcr();
		puts("- help: this help"); putcr();
		putcr();
		return;
	}

	puts("Invalid command");
	putcr();
}

void callback(unsigned char c) {
	// If a shift key is pressed
	if (c == 0) return;

	// The user pressed Enter. Processing the command buffer
	if (c == '\n') {
		buffer[buffer_end] = 0;
		putcr();
		if (buffer_end != 0) process_command();
		prompt();
		buffer_end = 0;
		return;
	}

	// The user pressed backspace
	if (c == '\b') {
		if (buffer_end == 0) return;
		backspace();
		buffer[buffer_end--] = 0;
		return;
	}

	// If the command buffer is full, stop
	if (buffer_end >= BUFFER_SIZE - 1) return;

	// Otherwise fill the buffer
	buffer[buffer_end++] = c;

	// And print the character on the screen
	putc(c);
	print_hex(c, 0, 75);
}

void start_shell() {
	putcr();
	prompt();
	buffer_end = 0;
	keyboard_set_callback(callback);
}
