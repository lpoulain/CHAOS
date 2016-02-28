#ifndef __DISK_H
#define __DISK_H

#include "libc.h"

// This is simular to the fat_DirEntry
// (same size), but with slight differences
// In particular, it contains
typedef struct __attribute__((packed)) {
	uint8 attributes;
	unsigned char filename[13];
	uint16 creation_time;
	uint16 creation_date;
	uint16 access_date;
	uint16 has_longname;
	uint16 modified_time;
	uint16 modified_date;
	uint16 address;
	uint size;	
} DirEntry;

typedef struct {
	DirEntry info;
	uint dir_entry_sector;
	uint dir_entry_offset;
	char filename[256];
	unsigned char *body;
} File;

void disk_ls(uint cluster, DirEntry *dir_index);
int disk_cd(unsigned char *dir_name, DirEntry *dir_index, char *path);
void disk_load_file_index();
void disk_get_filename(DirEntry *f, unsigned char *filename);
uint8 disk_has_long_filename(DirEntry *f);
char *disk_get_long_filename(DirEntry *f);
uint8 disk_is_dir_entry_valid(DirEntry *f);
int disk_load_file(const char *filename, uint dir_cluster, DirEntry *dir_index, File *f);
int disk_write_file(File *f);
uint8 disk_skip_n_entries(DirEntry *f);
uint8 disk_is_directory(DirEntry *f);

#define DISK_CMD_OK				-1
#define DISK_ERR_DOES_NOT_EXIST	-2
#define DISK_ERR_NOT_A_DIR		-3
#define DISK_ERR_NOT_A_FILE		-4
#define DISK_FILE_EMPTY			-5

#define ROOT_DIR_CLUSTER		2

#endif
