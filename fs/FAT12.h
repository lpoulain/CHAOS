#ifndef __FAT12_H
#define __FAT12_H

#include "disk.h"

typedef struct __attribute__((packed)) {
	unsigned char name[8];
	unsigned char ext[3];
	uint8 attributes;
	uint8 reserved;
	uint8 creation_10thsec;
	uint16 creation_time;
	uint16 creation_date;
	uint16 access_date;
	uint16 empty;
	uint16 modified_time;
	uint16 modified_date;
	uint16 address;
	uint size;
} fat_DirEntry;

void FAT12_load_table();
uint16 FAT12_read_entry(uint idx);
void FAT12_read_directory(uint cluster, DirEntry *entries);
void FAT12_read_file(DirEntry *f, char *buffer);
void FAT12_write_file(DirEntry *f, char *buffer);

#endif
