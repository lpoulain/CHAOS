#include "libc.h"
#include "disk.h"

#define ELF_SECTION_TEXT            0
#define ELF_SECTION_STRING          1
#define ELF_SECTION_DEBUG_INFO      2
#define ELF_SECTION_DEBUG_LINE      3
#define ELF_SECTION_DEBUG_ABBREV    4
#define ELF_SECTION_DEBUG_STR       5
#define ELF_SECTION_REL_TEXT        6

typedef struct
{
    uint8  e_ident[16];     // Should start with ELF
    uint16 e_type;
    uint16 e_machine;
    uint e_version;
    uint e_entry;           // The entry point (where to execute the program once it's loaded)
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

typedef struct
{
    unsigned char *start;
    unsigned char *end;
} ElfSection;

typedef struct
{
    File *file;
    ElfHeader *header;
    uint initial_code_address;
    int relocation_offset;
    ElfSection section[7];
} Elf;

Elf *elf_load(const char *filename, uint dir_cluster);
void elf_exec(const char *filename, int argc, char **argv);
void elf_relocate_addresses(Elf *elf);
