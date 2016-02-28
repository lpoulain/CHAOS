#include "libc.h"
#include "elf.h"
#include "debug_line.h"
#include "dwarf.h"

typedef struct __attribute__((packed)) {
	uint length;
	uint16 version;
	uint header_length;
	uint8 min_instruction_length;
	uint8 default_is_stmt;
	sint8 line_base;
	uint8 line_range;
	uint8 opcode_base;
	uint8 std_opcode_lengths[12];
} DebugLineHeader;


int debug_line_get_path(DebugLineHeader *header, void *ptr, StackFrame *frame, Elf *elf) {
	unsigned char *dirs = (unsigned char*)header + sizeof(DebugLineHeader);
	unsigned char *files = dirs;
	unsigned char *end = (unsigned char *)header + header->length;

	while (files[0] != 0) {
//		debug(files);
		files += strlen(files) + 1;
	}

	files++;

//	debug(files);

	unsigned char *dwarf_address = 0;
	uint dwarf_op_index = 0;
	uint dwarf_file = 1;
	uint dwarf_line = 1;
	uint dwarf_column = 0;
	uint8 dwarf_is_stmt = header->default_is_stmt;
	uint8 dwarf_basic_block = 0;
	uint8 dwarf_end_sequence = 0;
	uint8 dwarf_prologue_end = 0;
	uint8 dwarf_epilogue_begin = 0;
	uint dwarf_isa = 0;
	uint dwarf_discriminator = 0;

	uint8 old_opcode = 0;
	char *addr_opcode;
	uint nb_bytes, old_line = 1;

	unsigned char *opcode = (unsigned char *)header + header->header_length + 10;
	while (opcode < end && dwarf_address < (unsigned char *)ptr && !dwarf_epilogue_begin) {
		old_line = dwarf_line;
		
		addr_opcode = opcode;
		// Extended opcode
		if (*opcode == 0) {
			old_opcode = *(opcode + 2);

			switch(*(opcode + 2)) {
				// DW_LNE_end_sequence
				case 1:
					dwarf_end_sequence = 1;
					break;
				// DW_LNE_set_address
				case 2:
					dwarf_address = *((char **)(opcode + 3));
					break;
				// DW_LNE_define_file
				case 3:
					printf("Extended opcode not implemented %d\n", opcode);
					break;
				// DW_LNE_set_discriminator
				case 4:
					dwarf_discriminator = *((uint*)(opcode + 3));
					break;

			}
			opcode += *(opcode+1) + 2;
		}
		// Special opcode
		else if (*opcode > 12) {
			old_opcode = *opcode - header->opcode_base;
			dwarf_address += ((*opcode - header->opcode_base) / header->line_range) * header->min_instruction_length;
			dwarf_line += header->line_base + (*opcode - header->opcode_base) % header->line_range;
			opcode++;
		}
		// Standard opcodes
		else {
			old_opcode = *opcode;
			nb_bytes = 0;
			switch(*opcode) {
				// DW_LNS_copy
				case 1:
					dwarf_basic_block = 0;
					dwarf_end_sequence = 0;
					dwarf_prologue_end = 0;
					dwarf_epilogue_begin = 0;
					break;
				// DW_LNS_advance_PC
				case 2:
					dwarf_address += decodeULEB128(opcode + 1, &nb_bytes) * header->min_instruction_length;
					break;
				// DW_LNS_advance_line
				case 3:
					dwarf_line += decodeSLEB128(opcode + 1, &nb_bytes);
					break;
				// DW_LNS_set_line
				case 4:
					dwarf_line = decodeULEB128(opcode + 1, &nb_bytes);
					break;
				// DW_LNS_set_column
				case 5:
					dwarf_column = decodeULEB128(opcode + 1, &nb_bytes);
					break;
				// DW_LNS_negate_stmt
				case 6:
					dwarf_is_stmt = (1 - dwarf_is_stmt);
					break;
				// DW_LNS_set_basic_block
				case 7:
					dwarf_basic_block = 1;
					break;
				// DW_LNS_const_add_pc
				case 8:
					dwarf_address += ((255 - header->opcode_base) / header->line_range) * header->min_instruction_length;
					break;
				// DW_LNS_fixed_advanced_pc
				case 9:
					dwarf_address += decodeULEB128(opcode + 1, &nb_bytes);
					break;
				// DW_LNS_set_prologue_end
				case 10:
					dwarf_prologue_end = 1;
					break;
				// DW_LNS_set_epilogue_begin
				case 11:
					dwarf_epilogue_begin = 1;
					break;
				// DW_LNS_set_isa
				case 12:
					dwarf_isa = decodeULEB128(opcode + 1, &nb_bytes);
					break;
			}

			opcode += 1 + nb_bytes;
		}

		if (is_debug()) printf("[%x][%x] Opcode %d => Address=%x, line=%d       \n", addr_opcode, ((unsigned char*)addr_opcode - elf->section[ELF_SECTION_DEBUG_LINE].start), old_opcode, dwarf_address, dwarf_line);
	}

	if (dwarf_address > (unsigned char *)ptr) dwarf_line = old_line;

	char *filename = files;
	char *path = dirs;
	for (int i=0; i<0; i++) {
		files += strlen(files) + 2;
	}
//	debug(filename);
	filename = files + strlen(files) + 1;
	for (int i=0; i<(*filename) - 1; i++) {
		path += strlen(path) + 1;
	}
//	debug(path);
//	printf("%s/%s line %d       ", path, files, dwarf_line);
	frame->filename = files;
	frame->path = path;
	frame->line_number = dwarf_line;

	return 1;
}


DebugLineHeader *debug_line_find_block(void *ptr, Elf *elf) {
	DebugLineHeader *header = (DebugLineHeader*)elf->section[ELF_SECTION_DEBUG_LINE].start;
	DebugLineHeader *old_block = 0;
	unsigned char *end = elf->section[ELF_SECTION_DEBUG_LINE].end;
	int idx = 0;

	while (header->length > 0 && (unsigned char *)header < end) {
//		debug_i("Header at: ", header);
//		debug_i("Length: ", header->length);
//		debug_i("Header length: ", header->header_length);

		if (header->header_length + 13 < header->length) {
			uint *address = (uint*)((char*)header + header->header_length + 13);
//			debug_i("Address: ", *address);
			if (*address > (uint)ptr) return old_block;
		}

		old_block = header;
		header = (DebugLineHeader*) ((char*)header + header->length + 4);
//		debug_i("Length: ", header->length);
	}

	return old_block;
}

extern Elf *kernel_elf;

int debug_line_find_address(void *ptr, StackFrame *frame) {
	DebugLineHeader *header = debug_line_find_block(ptr, kernel_elf);
	if (header == 0) return 0;

//	printf("Found the block: %x\n", header);

	return debug_line_get_path(header, ptr, frame, kernel_elf);
}
