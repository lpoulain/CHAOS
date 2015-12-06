#include "kernel.h"
#include "display.h"
#include "shell.h"

// For now we are allocating it as a global
// variable
struct process processes[2];
struct process *current_process;

struct process *get_process() {
	return current_process;
}

void init_processes() {
	processes[0].pid = 0;
	processes[1].pid = 1;
	display_initialize(&processes[0].disp, 1, 12, 0x0f);
	display_initialize(&processes[1].disp, 13, 24, 0x0e);
}

void start_process(int nb) {
	current_process = &processes[nb];
	start_shell(&current_process->disp);
}

void switch_process() {
	int new_pid = 1 - current_process->pid;
	current_process = &processes[new_pid];
	set_cursor(&current_process->disp);
}
