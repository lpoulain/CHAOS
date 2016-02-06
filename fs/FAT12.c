#include "libc.h"
#include "heap.h"
#include "FAT12.h"
#include "display.h"
/*

sector = FAT * 8 + 32 + 1

1048576: boot sector

1049088: (sect 0): FAT 1
1053184 (sect 8): FAT 2
1054720 (sect 11): ???
1057280 (sect 16 / cluster 2): root directory

1065472 (sect 32 / cluster 0): begginging?

1073664 (sect 48 / cluster 2): file?

1085952 (sect 72 / cluster 5): file?
1090048 (sect 80 / cluster 6): boot/grub/grub.cfg
1106432 (sect 112 / cluster 10): kernel?.elf (part)

1167872 (sect 232): ?
1171968 (sect 240): ?

1180160 (sect 256 / cluster 28=0x1C): kernel?.elf (start)

1339904 (sect 568/ cluster 67=0x43): /boot/grub directory
1344000 (sect 576 / cluster 68=0x44): /boot directory

*/

extern unsigned char * read_sector(unsigned char *buf, uint addr);

uint16 FAT_table[4096];

void FAT12_load_table() {
	memset(&FAT_table, 0, 8192);
	unsigned char *buf = (char*)&FAT_table;

	// Read the FAT table
	for (int i=0; i<8; i++) {
		read_sector(buf + 512*i, i);
		buf += 512;
	}
}

uint16 FAT12_read_entry(uint idx) {
	uint offset = idx / 2;

	unsigned char *fat_raw = (unsigned char*)FAT_table + offset*3;

	// Convert it to an array
	uint16 fat_entry;

	if (idx % 2 == 0) {
		fat_entry = (uint16)fat_raw[0] + ((uint16)(fat_raw[1] & 0x0F) << 8);
	} else {
		fat_entry = (((uint16)fat_raw[2]) << 4) + ((uint16)(fat_raw[1] & 0xF0)) / 16;
	}

	return fat_entry;
}

void FAT12_read_file(DirEntry *f, char *buf) {
	void FAT12_load_table();

	uint16 fat_entry = f->address;

//	debug_i("FAT entry: ", fat_entry);
	while (fat_entry != 0 && fat_entry < (uint16)0xFF) {
		for (int i=0; i<8; i++) {
			read_sector(buf, fat_entry*8 + 32 + i);
			buf += 512;
		}
		fat_entry = FAT12_read_entry(fat_entry);
//		debug_i("FAT entry: ", fat_entry);
	}
}

// Reads the directory (fat_DirEntry format) and converts the entries
// into DirEntry
void FAT12_read_directory(uint cluster, DirEntry *dir_index) {
	char *buf = (char*)dir_index;
	char *buf_end = buf + 8192;
	uint16 filename_idx = 0;
//	debug_i("FAT: ", buf);

	fat_DirEntry *file_index = (fat_DirEntry*)dir_index;

	// Loads the directory sectors from disk
	for (int i=0; i<8; i++) {
		read_sector(buf, cluster*8 + i);
		buf += 512;
	}

	// Converts the fat_DirEntry records info DirEntry
	// - Move the attribute to the first byte
	// - Copy the 
	// - Compute the long name if any
	int idx = 0;
	int LFN_entry_end = -1;
	char filename[256], c;
	int filename_len;
	unsigned char *str;

	for (idx=0; file_index[idx].attributes != 0 && idx < 2014; idx++) {

		if (file_index[idx].name[0] == 0 ||		// unallocated
			file_index[idx].name[0] == 0xE5) {	// deleted
				file_index[idx].name[0] = 0;
				continue;
		}

		// If this is an entry that stores a long name
		if (file_index[idx].attributes & 0xF == 0xF) {
			if (LFN_entry_end < 0) LFN_entry_end = idx;
			continue;
		}

		// Computes the 8.3 filename in a print-ready format
		for (filename_len=0; filename_len<8 && file_index[idx].name[filename_len] != 0x20; filename_len++) {
			c = file_index[idx].name[filename_len];
			if (c >= 'A' && c <= 'Z') filename[filename_len] = c - 'A' + 'a';
			else filename[filename_len] = c;
		}

		if (file_index[idx].ext[0] == 0x20) filename[filename_len] = 0;
		else {
			filename[filename_len++] = '.';
			for (int j=0; j<3 && file_index[idx].ext[j] != 0x20; j++) {
				c = file_index[idx].ext[j];
				if (c >= 'A' && c <= 'Z') filename[filename_len++] = c - 'A' + 'a';
				else filename[filename_len++] = c;
			}
			filename[filename_len] = 0;
		}

		// Move the attributes at the beginning of the entry
		// Stores the filename right after
		file_index[idx].name[0] = file_index[idx].attributes;
		strcpy( ((char*)&file_index[idx]) + 1, filename);

		// If the file has a long name, compute it
		if (LFN_entry_end >= 0) {
			filename_len = 0;

			// We need to go back the entries to recontrusct the name
			// The n-1 last LFN entries are to be taken completely
			for (int j=idx-1; j>LFN_entry_end; j--) {
				str = (char*)&file_index[j];
				for (int k=1; k<32; k+= 2) {
					// We skip a few bytes
					if (k == 11) k += 3;
					if (k == 26) k += 2;
					filename[filename_len++] = str[k];
				}
			}

			// The last LFN entry may not be complete
			str = (unsigned char*)&file_index[LFN_entry_end];
			for (int k=1; k<32 && str[k] != 00; k+= 2) {
				if (k == 11) k += 3;
				if (k == 26) k += 2;
				filename[filename_len++] = str[k];
			}

			filename[filename_len] = 0;

			// Stores it in the long filename entries
			filename_idx = (idx - LFN_entry_end);
			strcpy( ((char*)&file_index[LFN_entry_end]) + 2, filename);
			// First byte tells it's a long name
			// Second byte tells to skip the next filename_idx entries
			file_index[LFN_entry_end].name[0] = 0xFF;
			file_index[LFN_entry_end].name[1] = filename_idx;
			// Indicates the dir entry has a long name
			file_index[idx].empty = filename_idx;

			LFN_entry_end = -1;			
		}
	}

}
