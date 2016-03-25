#include "libc.h"
#include "kheap.h"
#include "kernel.h"
#include "shell.h"
#include "process.h"
#include "parser.h"
#include "compiler.h"
#include "display.h"
#include "display_vga.h"
#include "gui_window.h"
#include "disk.h"
#include "debug.h"
#include "debug_info.h"
#include "elf.h"
#include "pci.h"
#include "dhcp.h"
#include "network.h"

#define DISK_ERR_DOES_NOT_EXIST	-2

extern void (*mouse_show)();
extern void (*mouse_hide)();
extern void stack_dump();
extern void read_sectors();
extern void reboot();
extern int new_process;
extern int polling;
extern void context_switch();
extern void syscall();
extern void memory_print(Window *);
extern void font_viewer();

// This structure holds the information specific to a
// shell session (command history, current directory)
typedef struct {
	char cmd_history[100][BUFFER_SIZE];
	uint cmd_history_idx;
	uint dir_cluster;
	DirEntry *dir_index;
	char path[256];
} ShellEnv;

void prompt(Window *win, ShellEnv *env) {
	win->action->puts(win, "CHAOS");
	win->action->puts(win, env->path);
	win->action->puts(win, "> ");
}

void countdown(Window *win) {
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

extern unsigned char *kernel_debug_line;
extern unsigned char *kernel_debug_info;
extern unsigned char *kernel_debug_str;
extern void edit(DirEntry *current_dir, uint dir_cluster, const char *filename);
extern unsigned char * read_sector(unsigned char *buf, uint addr);
extern void write_sector(unsigned char *buf, uint addr);
typedef void (*function)(int, char **);

#define asm_rdmsr(reg) ({uint low,high;asm volatile("rdmsr":"=a"(low),"=d"(high):"c"(reg));low|((uint)high<<32);})
#define asm_wrmsr(reg,val) do{asm volatile("wrmsr"::"a"((uint)val),"d"((uint)((uint)val)),"c"(reg));}while(0)
extern void syscall_handler2();
extern void draw_background(int left_x, int right_x, int top_y, int bottom_y);

uint dump_mem_addr = 0;

void process_command(Window *win, ShellEnv *env) {

	// Clear screen
	if (!strcmp(win->buffer, "cls")) {
		win->action->cls(win);
		return;
	}

	if (!strcmp(win->buffer, "debug")) {
		if (switch_debug()) win->action->puts(win, "Debug is ON");
		else win->action->puts(win, "Debug is off");

		win->action->putcr(win);
		return;
	}

	if (!strcmp(win->buffer, "ls")) {
		disk_load_file_index();
		disk_ls(env->dir_cluster, env->dir_index);

		char long_filename[256];

		char filename[13];
		char *str;
		uint16 skip;
		const char *src;
		uint16 fat_entry;

		for (int idx=0; env->dir_index[idx].filename[0] != 0; idx++) {
			// In some cases we need to skip some entries
			// (deleted, empty, used for long filenames)
			skip = disk_skip_n_entries(&env->dir_index[idx]);
			if (skip > 0) {
				idx += (skip - 1);
				continue;
			}

			if (disk_is_directory(&env->dir_index[idx])) win->action->puts(win, "      <DIR>  ");
			else {
				win->action->putnb_right(win, env->dir_index[idx].size);
				win->action->puts(win, "  ");
			}

			// If the previous entry(ies) are LFN, display the long filename
			if (disk_has_long_filename(&env->dir_index[idx])) {
				win->action->puts(win, disk_get_long_filename(&env->dir_index[idx]));
			} else {
				win->action->puts(win, env->dir_index[idx].filename);
			}
			win->action->putcr(win);

//			disk_load_file(&dir[i], win);
		}
		return;
	}

	if (!strncmp(win->buffer, "cd ", 3)) {
		disk_ls(env->dir_cluster, env->dir_index);
		int result = disk_cd(win->buffer+3, env->dir_index, env->path);

		if (result == DISK_ERR_DOES_NOT_EXIST) {
			win->action->puts(win, "The directory does not exist");
			win->action->putcr(win);
			return;
		}
		
		if (result == DISK_ERR_NOT_A_DIR) {
			win->action->puts(win, "This is not a directory");
			win->action->putcr(win);
			return;
		}

		env->dir_cluster = result;
		win->action->putcr(win);
		return;
	}

	if (!strncmp(win->buffer, "cat ", 4)) {
		File f;

//		disk_ls(env->dir_cluster, env->dir_index);
		int result = disk_load_file(win->buffer+4, env->dir_cluster, env->dir_index, &f);

		if (result == DISK_ERR_DOES_NOT_EXIST) {
			win->action->puts(win, "The file does not exist");
			win->action->putcr(win);
			return;
		}

		if (result == DISK_ERR_NOT_A_FILE) {
			win->action->puts(win, "This is not a file");
			win->action->putcr(win);
			return;
		}

		if (result != DISK_CMD_OK) {
			win->action->puts(win, "Error");
			win->action->putcr(win);
			return;
		}

		win->action->puts(win, "File: ");
		win->action->puts(win, f.filename);
		win->action->putcr(win);

		char *start;
		for (int i=0; i<f.info.size; i++) {
			if (f.body[i] == 0x0A) {
				win->action->putcr(win);
			} else if (f.body[i] == 0x09) {
				win->action->puts(win, "    ");
			} else {
				win->action->putc(win, f.body[i]);
			}
		}
		win->action->putcr(win);

		kfree(f.body);

		return;
	}

	if (!strncmp(win->buffer, "load ", 5)) {
		File f;

		disk_ls(env->dir_cluster, env->dir_index);
		int result = disk_load_file(win->buffer+5, env->dir_cluster, env->dir_index, &f);

		if (result == DISK_ERR_DOES_NOT_EXIST) {
			win->action->puts(win, "The file does not exist");
			win->action->putcr(win);
			return;
		}

		if (result == DISK_ERR_NOT_A_FILE) {
			win->action->puts(win, "This is not a file");
			win->action->putcr(win);
			return;
		}

		if (result != DISK_CMD_OK) {
			win->action->puts(win, "Error");
			win->action->putcr(win);
			return;
		}		

		win->action->puts(win, "File loaded at: ");
		win->action->puti(win, (uint)f.body);
		win->action->putcr(win);
		return;
	}

	if (!strncmp(win->buffer, "edit ", 5)) {
		disk_ls(env->dir_cluster, env->dir_index);
		edit(env->dir_index, env->dir_cluster, win->buffer + 5);

		win->action->init(win, " Shell ");
		win->action->cls(win);

		return;
	}

	if (!strcmp(win->buffer, "fonts")) {
		font_viewer();
		return;
	}

	if (!strcmp(win->buffer, "pci")) {
		PCIDevice *devices = PCI_get_devices();
		while (devices) {
			printf_win(win,
					   "Bus %d Slot %d IRQ %d [%s]: %s\n",
					   devices->bus, devices->slot, devices->IRQ, devices->vendor_name, devices->device_name);
			devices = devices->next;
		}
		return;
	}

	if (!strcmp(win->buffer, "ifconfig")) {
		Network *network = network_get_info();

		printf_win(win, "MAC Address:     %X:%X:%X:%X:%X:%X\n", network->MAC[0], network->MAC[1], network->MAC[2], network->MAC[3], network->MAC[4], network->MAC[5]);
		uint8 *ip = (uint8*)&network->IPv4;
		printf_win(win, "Your IP address: %d,%d,%d,%d\n", ip[0], ip[1], ip[2], ip[3]);
		ip = (uint8*)&network->router_IPv4;
		printf_win(win, "Gateway:         %d,%d,%d,%d\n", ip[0], ip[1], ip[2], ip[3]);
		ip = (uint8*)&network->dns;
		printf_win(win, "DNS Server:      %d,%d,%d,%d\n", ip[0], ip[1], ip[2], ip[3]);
		ip = (uint8*)&network->subnet_mask;
		printf_win(win, "Subnet mask:     %d,%d,%d,%d\n", ip[0], ip[1], ip[2], ip[3]);
		return;
	}

	if (!strncmp(win->buffer, "cc ", 3)) {
		compile_formula(win, win->buffer+3, env->dir_cluster, env->dir_index);
		return;
	}	

	if (!strcmp(win->buffer, "help")) {
		win->action->puts(win, "Commands:"); win->action->putcr(win);
		win->action->puts(win, "- help: this help"); win->action->putcr(win);
		win->action->puts(win, "- cls: clear screen"); win->action->putcr(win);
		win->action->puts(win, "- stack: prints the stack trace"); win->action->putcr(win);
		win->action->puts(win, "- countdown: pegs the CPU for a few seconds (to test multitasking)"); win->action->putcr(win);
		win->action->puts(win, "- mem: main memory pointers"); win->action->putcr(win);
		win->action->puts(win, "- mem [hex address]: memory dump (to test paging)"); win->action->putcr(win);
		win->action->puts(win, "- heap: shows what is being allocated in the heap"); win->action->putcr(win);
		win->action->puts(win, "- cd, ls, cat: basic filesystem functions"); win->action->putcr(win);
		win->action->puts(win, "- run [exec]: executes a file"); win->action->putcr(win);
		win->action->puts(win, "- fonts: tests the system's proportional fonts"); win->action->putcr(win);
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
		Window *win_dbg = set_window_debug(win);
		stack_dump();
		set_window_debug(win_dbg);
		return;
	}

	if (!strcmp(win->buffer, "redraw")) {
		gui_redraw(win, win->left_x, win->right_x, win->top_y, win->bottom_y);
		return;
	}

	if (!strcmp(win->buffer, "reboot")) {
		reboot();
		return;
	}

	// Prints the main memory pointers
	if (!strcmp(win->buffer, "mem")) {
	    printf_win(win, "code:       %x\n", (uint)&code);
	    printf_win(win, "r/o data:   %x\n", (uint)&rodata);
	    printf_win(win, "data:       %x\n", (uint)&data);
    	printf_win(win, "bss:        %x\n", (uint)&bss);
    	printf_win(win, "debug_info: %x\n", (uint)&debug_info);
    	printf_win(win, "end:        %x\n", (uint)&end);
    	printf_win(win, "heap:       %x->%x %x->%x\n", kheap.start, kheap.end, kheap.page_start, kheap.page_end);
    	memory_print(win);
		return;
	}

	if (!strncmp(win->buffer, "alloc ", 6)) {
		uint nb = atoi(win->buffer + 6);
		void *res = kmalloc_pages(nb, "Testing");
		printf_win(win, "=> %x\n", res);
		kheap_print(win);
		return;
	}

	if (!strncmp(win->buffer, "free ", 5)) {
		uint nb = atoi_hex(win->buffer + 5);
		kfree((void*)nb);
		kheap_print(win);
		return;
	}

	if (!strcmp(win->buffer, "heap")) {
		kheap_print(win);
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

	if (!strncmp(win->buffer, "run ", 4)) {
		int idx = 4;
		int argc = 0;
		char* argv[10];
		char *filename = win->buffer + 4;

		while (win->buffer[idx] != ' ' && idx++ < 256);
		win->buffer[idx++] = 0;

		argc = 1;
		argv[0] = win->buffer + idx;

//		printf("[%s]\n", argv[0]);

		elf_exec(filename, argc, (char **)&argv);
		return;
	}

	// Parsing arithmetic operations
	Token *tokens;
	int res = parse(win->buffer, &tokens);
	if (res <= 0) {
		for (int i=0; i<7-res; i++) win->action->putc(win, ' ');
		win->action->putc(win, '^');
		win->action->putcr(win);
		win->action->puts(win, "Syntax error");
		win->action->putcr(win);
		parser_memory_cleanup(tokens);
		return;
	}

	if (tokens->code == PARSE_WORD && !strcmp(tokens->value, "bg") &&
		tokens->next != 0 && tokens->next->code == PARSE_NUMBER &&
		tokens->next->next != 0 && tokens->next->next->code == PARSE_NUMBER &&
		tokens->next->next->next != 0 && tokens->next->next->next->code == PARSE_NUMBER &&
		tokens->next->next->next->next != 0 && tokens->next->next->next->code == PARSE_NUMBER) {

		draw_background((int)tokens->next->value,
						(int)tokens->next->next->value,
						(int)tokens->next->next->next->value,
						(int)tokens->next->next->next->next->value);

		parser_memory_cleanup(tokens);
		return;
	}

	int value;
	error_reset();
	res = is_math_formula(tokens, 0, &value);
	parser_memory_cleanup(tokens);
	
	if (res > 0) {
		win->action->putnb(win, value);
		win->action->putcr(win);
		return;
	} else if (res < 0) {
		for (int i=0; i<7-res; i++) win->action->putc(win, ' ');
		win->action->putc(win, '^');
		win->action->putcr(win);
		if (error_get()[0] != 0) {
			win->action->puts(win, error_get());
			win->action->putcr(win);
		}
		return;
	}

	// If everything else fails
	win->action->puts(win, "Invalid command");
	win->action->putcr(win);
}

void process_char(unsigned char c, Window *win, ShellEnv *env) {
//	window *win = get_process_focus()->win;

	// If no keyboard key is captured do nothing
	if (c == 0) return;

	// Up or down arrow: retreive the current command
	if (c == 1 || c == 2) {
		uint idx = env->cmd_history_idx;
		idx += (c - 1) * 2 - 1;
		if (idx < 0) idx = 99;
		if (idx >= 100) idx = 0;

		if (env->cmd_history[idx][0] == 0 && win->buffer_end == 0) return;

		for (int i=0; i<win->buffer_end; i++) win->action->backspace(win);
		strcpy(win->buffer, env->cmd_history[idx]);
		win->buffer_end = strlen(win->buffer);
		win->action->puts(win, win->buffer);
		win->action->set_cursor(win);
		env->cmd_history_idx = idx;
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
	
			uint idx = env->cmd_history_idx;
			strcpy(env->cmd_history[idx], win->buffer);
			idx++;
			if (idx >= 100) idx = 0;
			env->cmd_history_idx = idx;

			process_command(win, env);
		}
		prompt(win, env);
		win->action->set_cursor(win);
		win->buffer_end = 0;
		return;
	}

	// The user pressed backspace
	if (c == '\b') {
		if (win->buffer_end == 0) return;
		win->action->backspace(win);
		win->action->set_cursor(win);
		win->buffer_end--;
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

void shell() {
	Window *win = current_process->win;
	win->action->init(win, " Shell ");

	// Setup the shell environment
	ShellEnv *env = (ShellEnv*)kmalloc(sizeof(ShellEnv));
	memset(env, 0, sizeof(ShellEnv));
	env->dir_index = (DirEntry*)kmalloc_pages(1, "Shell current dir");
	env->dir_cluster = 2;
	strcpy(env->path, "");

	prompt(win, env);
	if (win == window_focus) win->action->set_cursor(win);
	win->buffer_end = 0;

	for (;;) {
		unsigned char c = getch();
		mouse_hide();
		process_char(c, win, env);
		mouse_show();
	}

	kfree(env);
}
