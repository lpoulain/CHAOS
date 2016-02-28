#include "libc.h"
#include "kheap.h"
#include "disk.h"
#include "elf.h"
#include "kernel.h"

typedef struct {
	uint offset;
	uint info;
} ElfRelocation;

typedef void (*function)(int, char **);

void elf_relocate_addresses(Elf *elf) {
	uint *ptr;
//    unsigned char *ptr_to_ptr;

    elf->relocation_offset = (int)elf->section[ELF_SECTION_TEXT].start - elf->initial_code_address;

	for (ElfRelocation *entry = (ElfRelocation*)elf->section[ELF_SECTION_REL_TEXT].start;
         entry < (ElfRelocation*)elf->section[ELF_SECTION_REL_TEXT].end;
         entry++) {
//		printf("Offset: %x, info: %x (%d)\n", entry->offset, entry->info, entry->info & 0xFF);
		if ((entry->info & 0xFF) == 01) {
			ptr = (uint*)(elf->section[ELF_SECTION_TEXT].start + entry->offset - elf->initial_code_address);
//			printf("Pointer to relocate: %x (%x + %x - %x)\n", ptr, elf->section[ELF_SECTION_TEXT].start, entry->offset, elf->initial_code_address);
//			memcpy(&ptr, ptr_to_ptr, 4);
//			printf("Old pointer: %x\n", *ptr);
			*ptr = *ptr + elf->relocation_offset;
//			printf("New pointer: %x\n", *ptr);
//			memcpy(ptr_to_ptr, &ptr, 4);
		}
	}
}

Elf *elf_load(const char *filename, uint dir_cluster) {
    File *f = (File*)kmalloc(sizeof(File));

    disk_load_file_index();
    DirEntry *dir_index = (DirEntry*)kmalloc_pages(1, "Root dir to load ELF");
    disk_load_file(filename, dir_cluster, dir_index, f);
    kfree(dir_index);

    if (strncmp(f->body + 1, "ELF", 3)) {
        printf("Invalid ELF binary at %x\n", f->body);
        return 0;
    }

    Elf *elf = (Elf*)kmalloc(sizeof(Elf));
    memset(elf, 0, sizeof(Elf));

    elf->file = f;
    elf->header = (ElfHeader*)f->body;

    ElfSectionHeader *sections = (ElfSectionHeader*)((uint)elf->header + elf->header->e_shoff);
    ElfSectionHeader *string_table_section = sections + elf->header->e_shstrndx;

    elf->section[ELF_SECTION_STRING].start = (unsigned char *)(elf->header) + string_table_section->sh_offset;

    // Go through all the sections and look for the .debug_line section
    // Once found, set kernel_debug_line and kernel_debug_line_size
    ElfSectionHeader *section = sections;
    int section_number;
    char *section_name;

    for (int idx = 0; idx < elf->header->e_shnum; idx++) {

        if (section->sh_name) {
            section_number = -1;
            section_name = elf->section[ELF_SECTION_STRING].start + section->sh_name;

            if (!strcmp(section_name, ".text")) {
                section_number = ELF_SECTION_TEXT;
                elf->initial_code_address = section->sh_addr;
            }
            else if (!strcmp(section_name, ".debug_line")) section_number = ELF_SECTION_DEBUG_LINE;
            else if (!strcmp(section_name, ".debug_info")) section_number = ELF_SECTION_DEBUG_INFO;
            else if (!strcmp(section_name, ".debug_abbrev")) section_number = ELF_SECTION_DEBUG_ABBREV;
            else if (!strcmp(section_name, ".debug_str")) section_number = ELF_SECTION_DEBUG_STR;
            else if (!strcmp(section_name, ".rel.text")) section_number = ELF_SECTION_REL_TEXT;

            if (section_number >= 0) {
                elf->section[section_number].start = (unsigned char *)(elf->header) + section->sh_offset;
                elf->section[section_number].end = (unsigned char *)(elf->header) + section->sh_offset + section->sh_size;
//                printf("Section %s: %x -> %x\n", section_name, elf->section[section_number].start, elf->section[section_number].end);
            }
        }

        section++;
    }

    return elf;
}

void elf_exec(const char *filename, int argc, char **argv) {
    DirEntry *dir_index = (DirEntry*)kmalloc_pages(1, "Root dir to load ELF");

    Elf *elf = elf_load(filename, ROOT_DIR_CLUSTER);
    if (!elf) return;

    elf_relocate_addresses(elf);

//    printf("Code section: %x\n", elf->section[ELF_SECTION_TEXT].start);

    function fct = (function)(elf->header->e_entry + elf->relocation_offset);
//    printf("main(): %x\n", fct);
    fct(argc, argv);

    kfree(dir_index);
    kfree(elf->file);
    kfree(elf->header);
    kfree(elf);
}
