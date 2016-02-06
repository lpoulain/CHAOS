#include "libc.h"
#include "heap.h"
#include "fat12.h"
#include "display.h"
#include "disk.h"


uint8 disk_is_directory(DirEntry *f) {
	return (f->attributes & 0x10);
}

void disk_ls(uint cluster, DirEntry *dir_index) {
	FAT12_read_directory(cluster, dir_index);
}

uint8 disk_skip_n_entries(DirEntry *f) {
	if (f->attributes == 0 ||			// unallocated
		f->attributes == 0xE5)			// deleted
		return 1;

	if (f->attributes == 0xFF) return f->filename[0];

	return 0;
}

DirEntry *find_dir_entry(unsigned char *filename_requested, DirEntry *dir_index) {
	int idx = 0, long_filename_idx;
	uint8 skip;

	for (idx=0; dir_index[idx].filename[0] != 0 && idx < 2048; idx++) {
		skip = disk_skip_n_entries(&dir_index[idx]);
		if (skip > 0) {
			idx += (skip - 1);
			continue;
		}

		long_filename_idx = dir_index[idx].has_longname;
		if (long_filename_idx != 0 && !strcmp( ((char*)&dir_index[idx - long_filename_idx]) + 2, filename_requested )) {
			return &dir_index[idx];
		}
		if (!strcmp(dir_index[idx].filename, filename_requested)) {
			return &dir_index[idx];
		}
	}

	return 0;
}

int disk_cd(unsigned char *dir_name, DirEntry *dir_index, char *path) {
	DirEntry *dir = find_dir_entry(dir_name, dir_index);

	// We check whether the directory exists and is a directory
	if (dir == 0) return DISK_ERR_DOES_NOT_EXIST;
	if (!disk_is_directory(dir)) return DISK_ERR_NOT_A_DIR;

	int i, len = strlen(path);
	if (!strcmp(dir_name, "..")) {
		int i;
		for (i=len; path[i] != '/' && i>=0; i--);
		path[i] = 0;
	} else if (strcmp(dir_name, ".")) {
		path[len] = '/';
		strcpy(path + len + 1, dir_name);
	}

	if (dir->address == 0) return 2;
	return dir->address + 4;
}

void disk_load_file_index() {
	FAT12_load_table();
}

char *disk_load_file_old(DirEntry *f, Window *win) {
	if (f->size == 0 || disk_is_directory(f)) return 0;

	uint size = f->size;
	uint16 fat_entry = f->address;

	win->action->puti(win, fat_entry);

	fat_entry = FAT12_read_entry(fat_entry);
	while (fat_entry != 0 && fat_entry < (uint16)0xFF) {
		win->action->puts(win, "   ");
		win->action->puti(win, (uint)fat_entry);

		fat_entry = FAT12_read_entry(fat_entry);
	}
	win->action->putcr(win);

	return 0;
}

uint8 disk_is_dir_entry_valid(DirEntry *f) {
	if (f->attributes == 0 ||			// unallocated
		f->attributes == 0xE5)			// deleted
		return 0;

	return 1;
}

uint8 disk_has_long_filename(DirEntry *f) {
	return (f->has_longname != 0);
}

char *disk_get_long_filename(DirEntry *f) {
	if (f->has_longname == 0) return 0;

	f -= f->has_longname;
	return ((char*)f) + 2;
}

int disk_load_file(unsigned char *filename, DirEntry *dir_index, File *f) {
	DirEntry *entry = find_dir_entry(filename, dir_index);
	
	if (entry == 0) 				return DISK_ERR_DOES_NOT_EXIST;
	if (disk_is_directory(entry)) 	return DISK_ERR_NOT_A_FILE;

	if (entry->size > 0) {
		f->body = (char*)kmalloc_a( ((entry->size / 8192) + 1) * 8192, 0);
		FAT12_read_file(entry, f->body);
	}

	f->info = *entry;

	if (disk_has_long_filename(entry)) strcpy(f->filename, disk_get_long_filename(entry));
	else strcpy(f->filename, entry->filename);

	return DISK_CMD_OK;
}
