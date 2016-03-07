#include "libc.h"
#include "kheap.h"
#include "disk.h"
#include "display.h"
#include "process.h"

typedef struct line_t {
	unsigned char text[80];
	unsigned char length;
	struct line_t *next;
	struct line_t *prev;
} Line;

Line *new_line(Line *previous_line) {
	Line *line = (Line*)kmalloc(sizeof(Line));
	line->text[0] = 0;
	line->length = 0;
	line->next = 0;
	// If there is a previous line
	if (previous_line) {
		// If the previous line has a new line, insert the new line between the two
		if (previous_line->next != 0) {
			line->next = previous_line->next;
			line->next->prev = line;
		}
		previous_line->next = line;
	}
	line->prev = previous_line;
}

typedef struct {
	DirEntry *dir_index;
	File *file;
	uint dir_cluster;
	uint cursor_x;
	uint cursor_y;
	uint nb_rows;
	uint nb_cols;
	Line *lines;
	Line *current_line;
	char cursor_buffer;
	uint8 exit_flag;
} EditEnv;

void print_lines(EditEnv *env) {
	Line *line = env->lines;
	while (line) {
		printf("[%s]       \n", line->text);
		line = line->next;
	}

	printf("                    \n");
}

void process_edit_ctrl_commands(unsigned char c, Window *win, EditEnv *env) {
	Line *line;

	switch(c) {
		case 'c':
			env->exit_flag = 1;
			break;
		case 's':
			line = env->lines;
			uint file_length = 0, total = env->file->info.size;
			if (total % 512 != 0) total = total - (total % 512) + 512;
			unsigned char *body = env->file->body;
			memset(body, 0, total);

			while (line) {
				strcpy(body, line->text);
				body += line->length;
				*body++ = 0x0A;
				line = line->next;
			}
			*(--body) = 0x00;
			env->file->info.size = (uint)(body - env->file->body);
			disk_write_file(env->file);
			break;
	}
}

void process_char_edit(unsigned char c, Window *win, EditEnv *env) {
	// If no keyboard key is captured do nothing
	if (c == 0) return;

	// Ctrl is pressed
	if (c & 0x80) {
		c -= 0x80;

		if (c >= 'A' && c <= 'Z') c -= ('A' - 'a');

		process_edit_ctrl_commands(c, win, env);
		return;
	}

	Line *line = env->current_line;
	win->action->printc(win, env->cursor_buffer, env->cursor_x, env->cursor_y);

	// Arrow keys: move the cursor around
	if (c >= 1 && c <= 4) {
		if (c == 1) {
			if (env->cursor_y > 0 && line->prev) {
				env->cursor_y--;
				line = line->prev;
			}
		}
		else if (c == 2) {
			if (line->next) {
				env->cursor_y++;
				line = line->next;
			}
		}
		else if (c == 3) {
			if (env->cursor_x > 0) env->cursor_x--;
		}
		else {
			if (env->cursor_x < line->length) env->cursor_x++;
		}

		env->current_line = line;
		if (env->cursor_x > line->length) env->cursor_x = line->length;
		env->cursor_buffer = line->text[env->cursor_x];
		if (env->cursor_buffer == 0) env->cursor_buffer = ' ';
		win->action->cursor(win, env->cursor_x, env->cursor_y);
		return;
	}

	if (c == '\n') {
		// Create a new line
		line = new_line(env->current_line);
		strcpy(line->text, line->prev->text + env->cursor_x);
		line->length = strlen(line->text);
		line->prev->text[env->cursor_x] = 0;
		line->prev->length = env->cursor_x;

		// Erase the rest of the current line
		for (int i=env->cursor_x; i<env->nb_cols; i++)
			win->action->printc(win, ' ', i, env->cursor_y);

		// Carriage return
		env->cursor_y++;
		env->cursor_x = 0;
		env->cursor_buffer = line->text[0];
		if (!env->cursor_buffer) env->cursor_buffer = ' ';

		// Prints
		win->action->scroll_down(win, env->cursor_y);
		win->action->print(win, line->text, env->cursor_x, env->cursor_y);
		win->action->cursor(win, env->cursor_x, env->cursor_y);
		env->current_line = line;
//		print_lines(env);
		return;
	}

	if (c == '\b') {
		if (env->cursor_x == 0 && env->cursor_y == 0) return;

		// Beginning of the line - we need to merge two lines
		if (env->cursor_x == 0) {
			Line *old_line = line;
			line = line->prev;
			line->next = old_line->next;

			strcpy(line->text + line->length, old_line->text);
			line->length += old_line->length;
			kfree(old_line);
		}
		else
		// Same line
		{
			memcpy(line->text + env->cursor_x - 1, line->text + env->cursor_x, line->length - env->cursor_x);
			env->cursor_x--;
			line->length--;
			win->action->print(win, line->text + env->cursor_x, env->cursor_x, env->cursor_y);
			win->action->printc(win, ' ', line->length, env->cursor_y);
			win->action->cursor(win, env->cursor_x, env->cursor_y);
		}

		return;
	}

	// Copies the rest of the line
	for (int i=line->length - env->cursor_x + 1; i>= 0; i--)
		line->text[env->cursor_x + i] = line->text[env->cursor_x + i -1];
//	memcpy(line->text + env->cursor_x + 1, line->text + env->cursor_x, line->length - env->cursor_x);
//	line->text[++line->length] = 0;
	++line->length;
	win->action->print(win, line->text + env->cursor_x + 1, env->cursor_x + 1, env->cursor_y);

	// Prints the new character
	line->text[env->cursor_x] = c;
	win->action->printc(win, c, env->cursor_x, env->cursor_y);
	env->cursor_x++;

	// Redraw the cursor
	env->cursor_buffer = line->text[env->cursor_x];
	if (!env->cursor_buffer) env->cursor_buffer = ' ';
	win->action->cursor(win, env->cursor_x, env->cursor_y);
}

int editor_load_file(const char *filename, uint dir_cluster, Window *win, EditEnv *env) {
	disk_ls(dir_cluster, env->dir_index);
	int result = disk_load_file(filename, dir_cluster, env->dir_index, env->file);

	if (result == DISK_CMD_OK) return 0;

	if (result == DISK_ERR_DOES_NOT_EXIST) {
		win->action->puts(win, "The file does not exist");
		win->action->putcr(win);
		return -1;
	}

	if (result == DISK_ERR_NOT_A_FILE) {
		win->action->puts(win, "This is not a file");
		win->action->putcr(win);
		return -1;
	}

	win->action->puts(win, "Error");
	win->action->putcr(win);
	return -1;
}

void editor_parse_file(EditEnv *env) {
	unsigned char *body = env->file->body;
	unsigned char *body_end = body + env->file->info.size;

	env->lines = new_line(0);
	env->current_line = env->lines;

	while (body < body_end) {
		env->current_line->length = 0;
		while (*body != 0x0A && body < body_end) {
			env->current_line->text[env->current_line->length++] = *body++;
		}
		env->current_line->text[env->current_line->length] = 0;
		body++;

		env->current_line = new_line(env->current_line);
	}

	env->current_line = env->lines;
}

void edit(DirEntry *current_dir, uint dir_cluster, const char *filename) {
	Window *win = current_process->win;

	// Setup the shell environment
	EditEnv *env = (EditEnv*)kmalloc(sizeof(EditEnv));
	env->dir_index = current_dir;
	env->file = (File*)kmalloc(sizeof(File));
	env->cursor_x = 0;
	env->cursor_y = 0;
	env->nb_cols = win->action->max_x_chars(win);
	env->nb_rows = win->action->max_y_chars(win);
	env->cursor_buffer = ' ';
	env->exit_flag = 0;

	// Try to load the file
	int result = editor_load_file(filename, dir_cluster, win, env);
	if (result < 0) return;

	// Parses the file
	editor_parse_file(env);

	// Setup the window title
	win->action->init(win, " Edit ");
	win->action->cls(win);

	// Displays the text
	for (int i=0; i<env->nb_rows; i++) {
		win->action->print(win, env->current_line->text, 0, env->cursor_y++);
		env->current_line = env->current_line->next;
		if (!env->current_line) break;
	}
	env->current_line = env->lines;
	env->cursor_y = 0;

	if (win == window_focus) win->action->cursor(win, env->cursor_x, env->cursor_y);
	win->buffer_end = 0;

	while (!env->exit_flag) {
		unsigned char c = getch();
		mouse_hide();
		process_char_edit(c, win, env);
		mouse_show();
	}

	// Deallocating the memory
	Line *old_line, *line = env->lines;
	while (line) {
		old_line = line;
		line = line->next;
		kfree(old_line);
	}
	kfree(env->file->body);
	kfree(env->file);
	kfree(env);
}
