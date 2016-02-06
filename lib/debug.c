#include "libc.h"
#include "heap.h"
#include "disk.h"
#include "debug_info.h"
#include "display.h"

typedef struct
{
    uint8  e_ident[16];
    uint16 e_type;
    uint16 e_machine;
    uint e_version;
    uint e_entry;
    uint e_phoff;
    uint e_shoff;
    uint e_flags;
    uint16 e_ehsize;
    uint16 e_phentsize;
    uint16 e_phnum;
    uint16 e_shentsize;
    uint16 e_shnum;
    uint16 e_shstrndx;
} ElfHeader;

typedef struct
{
    uint sh_name;
    uint sh_type;
    uint sh_flags;
    uint sh_addr;
    uint sh_offset;
    uint sh_size;
    uint sh_link;
    uint sh_info;
    uint sh_addralign;
    uint sh_entsize;
} ElfSectionHeader;

unsigned char *symbols;
unsigned char *kernel_debug_line;
uint kernel_debug_line_size;
unsigned char *kernel_debug_info;
uint kernel_debug_info_size;
unsigned char *kernel_debug_abbrev;
uint kernel_debug_abbrev_size;
unsigned char *kernel_debug_str;

uint8 debug_flag = 0;

uint8 switch_debug() {
    debug_flag = 1 - debug_flag;
    return debug_flag;
}

uint8 is_debug() {
    return debug_flag;
}

// Loads the kernel symbols in memory
void init_debug() {
    DirEntry *dir_index = (DirEntry*)kmalloc(sizeof(DirEntry), 0);
    File f;
    disk_load_file_index();
    disk_ls(2, dir_index);

    // Loads the kerney symbol file in memory
    if (display_mode() == VGA_MODE)
        disk_load_file("kernel_v.sym", dir_index, &f);
    else
        disk_load_file("kernel.sym", dir_index, &f);

    symbols = f.body;
    
    // Find the section table
    // This tells where are all the sections in the ELF binary
    ElfHeader *header = (ElfHeader*)symbols;
    ElfSectionHeader *sections = (ElfSectionHeader*)(symbols + header->e_shoff);

    ElfSectionHeader *string_table_section = sections + header->e_shstrndx;
    char *string_table = symbols + string_table_section->sh_offset;
//    debug_i("String table:", string_table);

    // Go through all the sections and look for the .debug_line section
    // Once found, set kernel_debug_line and kernel_debug_line_size
    ElfSectionHeader *section = sections;
    for (int idx = 0; idx < header->e_shnum; idx++) {
        if (section->sh_name) {
//            debug_i((char*)string_table + section->sh_name, section->sh_offset);
            if (!strcmp((char*)string_table + section->sh_name, ".debug_info")) {
                kernel_debug_info = (unsigned char*)symbols + section->sh_offset;
                kernel_debug_info_size = section->sh_size;
            }
            else if (!strcmp((char*)string_table + section->sh_name, ".debug_line")) {
                kernel_debug_line = (unsigned char*)symbols + section->sh_offset;
                kernel_debug_line_size = section->sh_size;
//                debug_i(".debug_line: ", kernel_debug_line);
            }
            else if (!strcmp((char*)string_table + section->sh_name, ".debug_abbrev")) {
                kernel_debug_abbrev = (unsigned char*)symbols + section->sh_offset;
                kernel_debug_abbrev_size = section->sh_size;
//                debug_i(".debug_abbrev: ", kernel_debug_abbrev);
            }
            else if (!strcmp((char*)string_table + section->sh_name, ".debug_str")) {
                kernel_debug_str = (unsigned char*)symbols + section->sh_offset;
            }
        }
        section++;
    }

    debug_info_load();
}
