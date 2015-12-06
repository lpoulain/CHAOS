#include "kernel.h"
#include "libc.h"
#include "keyboard.h"
#include "display.h"
#include "process.h"

#define BUFFER_SIZE 128

unsigned char buffer[BUFFER_SIZE];
int buffer_end;

void prompt(struct display *disp) {
	puts(disp, "CHAOS> ");
}

void process_command(struct display *disp) {
	if (!strcmp(buffer, "cls")) {
		cls(disp);
		return;
	}

	if (!strcmp(buffer, "help")) {
		puts(disp, "Commands:"); putcr(disp);
		puts(disp, "- cls: clear screen"); putcr(disp);
		puts(disp, "- help: this help"); putcr(disp);
		putcr(disp);
		return;
	}

	puts(disp, "Invalid command");
	putcr(disp);
}

void callback(unsigned char c) {
	struct display *disp = &get_process()->disp;

	// If a shift key is pressed
	if (c == 0) return;

	// The user pressed Enter. Processing the command buffer
	if (c == '\n') {
		buffer[buffer_end] = 0;
		putcr(disp);
		if (buffer_end != 0) process_command(disp);
		prompt(disp);
		buffer_end = 0;
		return;
	}

	// The user pressed backspace
	if (c == '\b') {
		if (buffer_end == 0) return;
		backspace(disp);
		buffer[buffer_end--] = 0;
		return;
	}

	// If the command buffer is full, stop
	if (buffer_end >= BUFFER_SIZE - 1) return;

	// Otherwise fill the buffer
	buffer[buffer_end++] = c;

	// And print the character on the screen
	putc(disp, c);
	print_hex(c, 0, 75);
}

void start_shell(struct display *disp) {
	puts(disp, "Welcome to CHAOS (CHrist, Another OS)!");
	putcr(disp);
	prompt(disp);
	buffer_end = 0;
	keyboard_set_callback(callback);
}
