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
#include "icmp.h"
#include "arp.h"
#include "dns.h"
#include "http.h"

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

// This structure holds the information specific to a
// shell session (command history, current directory)
typedef struct {
	char cmd_history[100][BUFFER_SIZE];
	uint cmd_history_idx;
	uint dir_cluster;
	DirEntry *dir_index;
	char path[256];
} ShellEnv;

typedef void (*shell_cmd)(Window *, ShellEnv *, Token *, uint length);

typedef struct {
	char *name;
	shell_cmd function;
	char *description;
} ShellCmd;

uint dump_mem_addr = 0;

////////////////////////////////////////////////////////////////////////
// All the Shell commands
////////////////////////////////////////////////////////////////////////

uint str2ip(uint8* string) {
	uint ip;
	uint8 *ip_ptr = (uint8*)&ip, *str_start = string;
	int str_pos = 0;

	for (int i=0; i<4; i++) {
		while ((string[str_pos] >= '0') &&
			   (string[str_pos] <= '9'))
			str_pos++;

		// Unknown character
		if (string[str_pos] != '.' &&
			(string[str_pos] != 0 || i < 3))
			return 0;

		// It's the end of the string and we don't have 4 digits
		if (string[str_pos] == 0 && i < 3) return 0;

		// Empty number between two separators
		if (str_pos <= (uint)str_start+1) return 0;

		string[str_pos] = 0;

		*ip_ptr++ = (uint8)atoi(str_start);
	}

	return ip;
}

void shell_help(Window *win, ShellEnv *env, Token *tokens, uint length);

void shell_mem(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length > 0 && tokens->code == PARSE_HEX) {
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


    printf_win(win, "code:       %x\n", (uint)&code);
    printf_win(win, "r/o data:   %x\n", (uint)&rodata);
    printf_win(win, "data:       %x\n", (uint)&data);
   	printf_win(win, "bss:        %x\n", (uint)&bss);
   	printf_win(win, "debug_info: %x\n", (uint)&debug_info);
   	printf_win(win, "end:        %x\n", (uint)&end);
   	printf_win(win, "heap:       %x->%x %x->%x\n", kheap.start, kheap.end, kheap.page_start, kheap.page_end);
   	memory_print(win);
}

void shell_heap(Window *win, ShellEnv *env, Token *tokens, uint length) {
	kheap_print(win);
}

void shell_heap_pages(Window *win, ShellEnv *env, Token *tokens, uint length) {
	kheap_print_pages(win);
}

void shell_cd(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length == 0 || tokens->code != PARSE_WORD) {
		printf_win(win, "Invalid directory name\n");
		return;
	}

	disk_ls(env->dir_cluster, env->dir_index);
	int result = disk_cd(tokens->value, env->dir_index, env->path);

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
}

void shell_load(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length == 0 || tokens->code != PARSE_WORD) {
		printf_win(win, "Invalid filename\n");
		return;
	}

	File f;

	disk_ls(env->dir_cluster, env->dir_index);
	int result = disk_load_file(tokens->value, env->dir_cluster, env->dir_index, &f);

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
}

void shell_cat(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length == 0 || tokens->code != PARSE_WORD) {
		printf_win(win, "Invalid filename\n");
		return;
	}

	File f;

//		disk_ls(env->dir_cluster, env->dir_index);
	int result = disk_load_file(tokens->value, env->dir_cluster, env->dir_index, &f);

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
}

void shell_cc(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length == 0 || tokens->code != PARSE_WORD) {
		printf_win(win, "Invalid filename\n");
		return;
	}

	compile_formula(win, tokens->value, env->dir_cluster, env->dir_index);
}

void shell_run(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length == 0 || tokens->code != PARSE_WORD) {
		printf_win(win, "Invalid filename\n");
		return;
	}

	if (length == 1 || tokens->next->code != PARSE_NUMBER) {
		printf_win(win, "You must pass a number as an argument\n");
		return;
	}

	int idx = 4;
	int argc = 0;
	char* argv[10];
	char *filename = win->buffer + 4;

	while (win->buffer[idx] != ' ' && idx++ < 256);
	win->buffer[idx++] = 0;

	argc = 1;
	argv[0] = win->buffer + idx;

	elf_exec(filename, argc, (char **)&argv);
}

void shell_edit(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length == 0 || tokens->code != PARSE_WORD) {
		printf_win(win, "Invalid filename\n");
		return;
	}

	disk_ls(env->dir_cluster, env->dir_index);
	edit(env->dir_index, env->dir_cluster, tokens->value);

	win->action->init(win, " Shell ");
	win->action->cls(win);
}

void shell_ls(Window *win, ShellEnv *env, Token *tokens, uint length) {
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
}

void shell_pci(Window *win, ShellEnv *env, Token *tokens, uint length) {
	PCIDevice *devices = PCI_get_devices();
	while (devices) {
		printf_win(win,
				   "Bus %d Slot %d IRQ %d [%s]: %s\n",
				   devices->bus, devices->slot, devices->IRQ, devices->vendor_name, devices->device_name);
		devices = devices->next;
	}
}

void shell_debug(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (switch_debug()) win->action->puts(win, "Debug is ON");
	else win->action->puts(win, "Debug is off");

	win->action->putcr(win);
}

void shell_redraw(Window *win, ShellEnv *env, Token *tokens, uint length) {
	gui_redraw(win, win->left_x, win->right_x, win->top_y, win->bottom_y);
}

void shell_cls(Window *win, ShellEnv *env, Token *tokens, uint length) {
	win->action->cls(win);
}

void shell_bg(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length < 4) {
		printf_win(win, "Need 4 numbers as arguments\n");
		return;
	}

	if (tokens->code == PARSE_NUMBER &&
		tokens->next->code == PARSE_NUMBER &&
		tokens->next->next->code == PARSE_NUMBER &&
		tokens->next->next->next->code == PARSE_NUMBER) {

		draw_background((int)tokens->value,
						(int)tokens->next->value,
						(int)tokens->next->next->value,
						(int)tokens->next->next->next->value);

		parser_memory_cleanup(tokens);
		return;
	}
}

void shell_fonts(Window *win, ShellEnv *env, Token *tokens, uint length) {
	font_viewer();
}

void shell_ifconfig(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length > 0 && tokens->code == PARSE_WORD && !strncmp(tokens->value, "r", 1)) {
		DHCP_send_packet();
		return;
	}

	Network *network = network_get_info();

	printf_win(win, "MAC Address:     %X:%X:%X:%X:%X:%X\n", network->MAC[0], network->MAC[1], network->MAC[2], network->MAC[3], network->MAC[4], network->MAC[5]);
	printf_win(win, "Your IP address: %i\n", network->IPv4);
	printf_win(win, "Gateway:         %i\n", network->router_IPv4);
	printf_win(win, "DNS Server:      %i\n", network->dns);
	printf_win(win, "Subnet mask:     %i\n", network->subnet_mask);
}

void shell_stack(Window *win, ShellEnv *env, Token *tokens, uint length) {
	Window *win_dbg = set_window_debug(win);
	stack_dump();
	set_window_debug(win_dbg);
}

void shell_countdown(Window *win, ShellEnv *env, Token *tokens, uint length) {
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

void shell_reboot(Window *win, ShellEnv *env, Token *tokens, uint length) {
	reboot();
}

void shell_dns(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length == 0) {
		DNS_print_table(win);
		return;
	}
	
	if (tokens->code != PARSE_WORD) {
		printf_win(win, "Invalid hostname\n");
		return;
	}

	uint ip = DNS_query(win->buffer + 4);
	printf_win(win, "=> %i\n", ip);
}

void shell_ping(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length >= 7 &&
		tokens->code == PARSE_NUMBER &&
		tokens->next->code == PARSE_COMMA &&
		tokens->next->next->code == PARSE_NUMBER &&
		tokens->next->next->next->code == PARSE_COMMA &&
		tokens->next->next->next->next->code == PARSE_NUMBER &&
		tokens->next->next->next->next->next->code == PARSE_COMMA &&
		tokens->next->next->next->next->next->next->code == PARSE_NUMBER) {

		uint8 ip[4] = {
			(uint8)(uint)tokens->value,
			(uint8)(uint)tokens->next->next->value,
			(uint8)(uint)tokens->next->next->next->next->value,
			(uint8)(uint)tokens->next->next->next->next->next->next->value
		};

		printf_win(win, "Pinging %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
		uint *ipv4 = (uint*)&ip;

		uint ps_id = getpid();
		ICMP_register_reply(ps_id);
		ICMP_send_packet(*ipv4, ps_id);
		uint8 status;

		for (int i=0; i<2000000000; i++) {
			status = ICMP_check_response(ps_id);
			if (status != ICMP_TYPE_ECHO_REQUEST) {
				if (status == ICMP_TYPE_ECHO_REPLY)
					printf_win(win, "PONG!\n");
				else if (status == ICMP_TYPE_ECHO_UNREACHABLE)
					printf_win(win, "Host unreachable\n");
				else
					printf_win(win, "Unknown response code: %d\n", status);

				ICMP_unregister_reply(ps_id);
				return;
			}
		}

		ICMP_unregister_reply(ps_id);
		printf_win(win, "Response timeout...\n");

		return;
	}

	printf_win(win, "Unknown IP address\n");
}

void shell_arp(Window *win, ShellEnv *env, Token *tokens, uint length) {
	ARP_print_table(win);
}

void shell_http(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length == 0) {
		printf_win(win, "Please pass a hostname\n");
		return;
	}

	if (tokens->code != PARSE_WORD) {
		printf_win(win, "Invalid hostname\n");
		return;
	}

	HTTP_get(win, tokens->value);
}

void shell_https(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length == 0) {
		HTTPS_get(win, "www.wikipedia.org");
//		printf_win(win, "Please pass a hostname\n");
		return;
	}

	if (tokens->code != PARSE_WORD) {
		printf_win(win, "Invalid hostname\n");
		return;
	}

	HTTPS_get(win, tokens->value);
}

void shell_addr(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length > 0 && tokens->code == PARSE_HEX) {
		StackFrame frame;

        uint fct_ptr = tokens->value;
        fct_ptr = 0x00118354;

        if (debug_line_find_address((void*)fct_ptr, &frame)) {
            debug_info_find_address((void*)fct_ptr, &frame);
            printf_win(win, "[%x] %s (%s/%s at line %d)  \n", fct_ptr, frame.function, frame.path, frame.filename, frame.line_number);
        }
        else
        	printf_win(win, "Not a known address\n");

		return;
	}

	printf_win(win, "Usage: addr [hex address]\n");
}

////////////////////////////////////////////////////////////////////////

#define NB_CMDS	27

ShellCmd commands[NB_CMDS] = {
	{ .name = "help",		.function = shell_help,			.description = "This help\n" },
	{ .name = "addr",		.function = shell_addr,			.description = "Displays the source code corresponding to the address\n" },
	{ .name = "arp",		.function = shell_arp,			.description = "Prints the IP->MAC translation table\n" },
	{ .name = "bg",			.function = shell_bg,			.description = "bg <nb> <nb> <nb> <nb>: draws a background rectangle [DEBUG]\n" },
	{ .name = "cat",		.function = shell_cat,			.description = "cat <filename>: prints the content of a file\n" },
	{ .name = "cc",			.function = shell_cc,			.description = "cc <filename>.f: compiles a formula into a native x86 executable\n" },
	{ .name = "cd",			.function = shell_cd,			.description = "cd <directory>: change directory\n" },
	{ .name = "cls",		.function = shell_cls,			.description = "Clears the screen\n" },
	{ .name = "countdown",	.function = shell_countdown,	.description = "Countdown (to test multitasking)\n" },
	{ .name = "debug",		.function = shell_debug,		.description = "Sets the debug mode on/off\n" },
	{ .name = "dns",		.function = shell_dns,			.description = "dns <hostname>: resolves a hostname into an IP address\n" },
	{ .name = "edit",		.function = shell_edit,			.description = "edit <filename>: file editor\n" },
	{ .name = "fonts",		.function = shell_fonts,		.description = "Tests the system proportional fonts (press Enter to exit)\n" },
	{ .name = "heap",		.function = shell_heap,			.description = "A detailed information of current heap allocations\n" },
	{ .name = "pages",		.function = shell_heap_pages,	.description = "A detailed information of current page allocations\n" },
	{ .name = "http",		.function = shell_http,			.description = "Sends an HTTP GET request\n" },
	{ .name = "https",		.function = shell_https,		.description = "Sends an HTTPS GET request (using TLS 1.2)\n" },
	{ .name = "ifconfig",	.function = shell_ifconfig,		.description = "Prints the network configuration\n" },
	{ .name = "ls",			.function = shell_ls,			.description = "Displays the files in the current directory\n" },
	{ .name = "load",		.function = shell_load,			.description = "load <filename>: loads a file into memory\n" },
	{ .name = "mem",		.function = shell_mem,			.description = "mem: shows the main memory addresses\nmem <hex>: memory dump\n" },
	{ .name = "pci",		.function = shell_pci,			.description = "Displays the available PCI devices\n" },
	{ .name = "ping",		.function = shell_ping,			.description = "Ping another host on the network\n" },
	{ .name = "reboot",		.function = shell_reboot,		.description = "Reboots CHAOS\n" },
	{ .name = "run",		.function = shell_run,			.description = "run <filename>: runs an ELF executable\n" },
	{ .name = "redraw",		.function = shell_redraw,		.description = "Redraws the current window\n" },
	{ .name = "stack",		.function = shell_stack,		.description = "Prints the current stack trace\n" },
};

void shell_help(Window *win, ShellEnv *env, Token *tokens, uint length) {
	if (length > 0 && tokens->code == PARSE_WORD) {
		for (int i=0; i<NB_CMDS; i++) {
			if (!strcmp(tokens->value, commands[i].name)) {
				printf_win(win, commands[i].description);
				return;
			}
		}
		printf_win(win, "Unknown command\n", tokens->value);
		return;
	}

	printf_win(win, "%s", commands[0].name);

	for (int i=1; i<NB_CMDS; i++) {
		printf_win(win, ", %s", commands[i].name);
	}

	printf_win(win, "\n");
}

void prompt(Window *win, ShellEnv *env) {
	printf_win(win, "CHAOS%s> ", env->path);
//	win->action->puts(win, "CHAOS ");
//	win->action->puts(win, env->path);
//	win->action->puts(win, "> ");
}


void process_command(Window *win, ShellEnv *env) {

	Token *tokens;
	uint length = 0;
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

	Token *token_tmp = tokens;
	while (token_tmp) {
		token_tmp = token_tmp->next;
		length++;
	}

	if (length >= 1 && tokens->code == PARSE_WORD) {
		char *command = tokens->value;

		for (int i=0; i<NB_CMDS; i++) {
			if (!strcmp(command, commands[i].name)) {
				commands[i].function(win, env, tokens->next, length-1);
				return;
			}
		}
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
	win->action->puts(win, "Invalid command - type 'help' for the available commands");
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
