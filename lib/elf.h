#include "libc.h"

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
