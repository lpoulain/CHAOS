#include "libc.h"
#include "kheap.h"
#include "disk.h"
#include "elf.h"
#include "kernel.h"

typedef struct {
	uint offset;
	uint info;
} ElfRelocation;

typedef void (*function)();

void relocate_addresses(unsigned char *text_section, uint text_section_address, int relocation_offset, ElfRelocation *relocation_table, uint relocation_table_size) {
	uint *ptr;
//	unsigned char *ptr_to_ptr;
	for (ElfRelocation *entry = relocation_table; entry < relocation_table + relocation_table_size; entry++) {
//		printf("Offset: %x, info: %x (%d)\n", entry->offset, entry->info, entry->info & 0xFF);
		if ((entry->info & 0xFF) == 01) {
			ptr = (uint*)(text_section + entry->offset - text_section_address);
//			printf("Pointer to relocate: %x (%x + %x - %x)\n", ptr, text_section, entry->offset, text_section_address);
//			memcpy(&ptr, ptr_to_ptr, 4);
//			printf("Old pointer: %x\n", *ptr);
			*ptr = *ptr + relocation_offset;
//			printf("New pointer: %x\n", *ptr);
//			memcpy(ptr_to_ptr, &ptr, 4);
		}
	}
}

void load_elf(const char *filename) {
//    DirEntry *dir_index = (DirEntry*)kmalloc(sizeof(DirEntry));
    DirEntry *dir_index = (DirEntry*)kmalloc_pages(2, "Root dir to load ELF");
    File f;
    disk_load_file_index();
    disk_ls(2, dir_index);

    disk_load_file(filename, dir_index, &f);
    kfree(dir_index);
    unsigned char *executable = f.body;

    // Find the section table
    // This tells where are all the sections in the ELF binary
    ElfHeader *header = (ElfHeader*)executable;
    ElfSectionHeader *sections = (ElfSectionHeader*)(executable + header->e_shoff);

    ElfSectionHeader *string_table_section = sections + header->e_shstrndx;
    char *string_table = executable + string_table_section->sh_offset;
//    debug_i("String table:", string_table);

    uint initial_code_address;
    unsigned char *text_section;
    unsigned char *rel_text_section;
    uint rel_text_size;

    // Go through all the sections and look for the .debug_line section
    // Once found, set kernel_debug_line and kernel_debug_line_size
    ElfSectionHeader *section = sections;
    for (int idx = 0; idx < header->e_shnum; idx++) {
        if (section->sh_name) {
//            debug_i((char*)string_table + section->sh_name, section->sh_offset);
            if (!strcmp((char*)string_table + section->sh_name, ".text")) {
                text_section = (unsigned char*)executable + section->sh_offset;
                initial_code_address = section->sh_addr;
            }
            else if (!strcmp((char*)string_table + section->sh_name, ".rel.text")) {
                rel_text_section = (unsigned char*)executable + section->sh_offset;
                rel_text_size = section->sh_size;
//                debug_i(".debug_line: ", kernel_debug_line);
            }
        }
        section++;
    }    

    int relocation_offset = (uint)text_section - initial_code_address;
    relocate_addresses(text_section, initial_code_address, relocation_offset, (ElfRelocation*)rel_text_section, rel_text_size / sizeof(ElfRelocation));

//    printf("Executable loaded on %x\n", executable);
//    printf("Code section: %x\n", text_section);

    function fct = (function)(header->e_entry + relocation_offset);
//    printf("main(): %x\n", fct);
    fct();

    kfree(executable);
}
