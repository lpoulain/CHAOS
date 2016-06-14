#include "libc.h"
#include "elf.h"
#include "dwarf.h"
#include "debug.h"

#define MAX_FUNCTION_RANGES	700

static uint8 DW_FORM_size[33] = {
	0, 4, 0, 0, 0, 2, 4, 8,
	0, 0, 0, 1, 0, 0, 4, 0,
	0, 1, 2, 4, 8, 0, 0, 4,
	0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static char *DW_FORM_name[33] = {
	"n/a", "addr", "n/a", "block2", "block4", "data2", "data4", "data8",
	"string", "block", "block1", "data1", "flag", "sdata", "strp", "udata",
	"ref_addr", "ref1", "ref2", "ref4", "ref8", "ref_udata", "indirect", "set_offset",
	"exprloc", "flag_present", "n/a", "n/a", "n/a", "n/a", "n/a", "n/a",
	"ref_sig8"
};

void cpp_unmanble(char *name) {
	int classname_length = 0, methodname_length=0, idx=3;
	while (name[idx] >= '0' && name[idx] <= '9') {
		classname_length *= 10;
		classname_length += name[idx] - '0';
		idx++;
	}
	for (int i=0; i<classname_length; i++)
		name[i] = name[i+idx];
	name[classname_length] = ':';
	name[classname_length+1] = ':';

	idx += classname_length;

	if (name[idx] == 'C') {
		name[classname_length+2] = 0;
	}
	else if (name[idx] == 'D') {
		name[classname_length+2] = '~';
		name[classname_length+3] = 0;
	}
	else if (name[idx] >= '0' && name[idx] <= '9') {
		while (name[idx] >= '0' && name[idx] <= '9') {
			methodname_length *= 10;
			methodname_length += name[idx] - '0';
			idx++;
		}
		
		for (int i=0; i<methodname_length; i++)
			name[classname_length+2+i] = name[i+idx];
		name[methodname_length+classname_length+2] = 0;
	}
	else name[classname_length+2] = 0;
}

typedef struct {
	char *name;
	unsigned char *low_pc;
	uint high_pc;
	uint address;
} FunctionRange;

static uint nb_function_ranges = 0;
static FunctionRange functionRanges[MAX_FUNCTION_RANGES];

typedef struct __attribute__((packed)) {
	uint length;
	uint16 version;
	uint abbrev_offset;
	uint8 pointer_size;
} CompilationUnitHeader;


// Macros to try to avoid wild reads/writes
#define CHECK_VALID_DIE(ptr) \
    if (ptr < elf->section[ELF_SECTION_DEBUG_INFO].start || ptr > elf->section[ELF_SECTION_DEBUG_INFO].end) {	\
    	printf("ERROR: DIE pointer outside of range: %x\n", ptr);							\
    	return;																		\
    }

#define CHECK_VALID_SCHEMA(ptr)	\
    if (ptr < elf->section[ELF_SECTION_DEBUG_ABBREV].start || ptr > elf->section[ELF_SECTION_DEBUG_ABBREV].end) {	\
    	printf("ERROR: schema pointer outside of range: %x\n", ptr);								\
    	return;																				\
    }

#define CHECK_VALID_ATTRIBUTE(ptr, sch) 								\
    if (*ptr < 0 || *ptr > 0x20) {								\
    	printf("ERROR: invalid attribute number (%d). Schema: %x/%x\n", *ptr, ptr, sch);	\
    	return;												\
    }

int debug_info_find_address(void *ptr, StackFrame *frame) {
//	printf("%d ranges starting at %x", nb_function_ranges, functionRanges);

	frame->function = "?";
	for (int i=0; i<nb_function_ranges; i++) {
		if ((unsigned char *)ptr >= functionRanges[i].low_pc && (unsigned char *)ptr < functionRanges[i].low_pc + functionRanges[i].high_pc) {
			if (functionRanges[i].name == 0) {
				printf("Problem idx %d [%x-%x]\n", i, functionRanges[i].low_pc, functionRanges[i].low_pc+functionRanges[i].high_pc);
				return 0;
			}
			frame->function = functionRanges[i].name;
//			printf("Found it: %s", functionRanges[i].name);
			return 1;
		}
	}

	return 0;
}


// Goes through the Debugging Information Entries (DIE)
// and finds the subprogram whose range covers the stack
void debug_info_load(Elf *elf) {
	for (int i=0; i<MAX_FUNCTION_RANGES; i++) {
		functionRanges[i].name = 0;
		functionRanges[i].low_pc = 0;
		functionRanges[i].high_pc = 0;
		functionRanges[i].address = 0;
	}

//	switch_debug();
	unsigned char *DIE_schema[300];
	unsigned char *end_section = elf->section[ELF_SECTION_DEBUG_INFO].end;
	unsigned char *die = elf->section[ELF_SECTION_DEBUG_INFO].start;
	unsigned char *start = die;

	uint nb_types;
	uint8 is_function_type;

	// Goes through the whole .debug_info section,
	// compilation unit by compilation unit
	while (die < end_section) {

		CompilationUnitHeader *header = (CompilationUnitHeader*)die;

		// Finds the schema address in mem
		// for each type of DIE
		unsigned char *schema = elf->section[ELF_SECTION_DEBUG_ABBREV].start + header->abbrev_offset;
		CHECK_VALID_SCHEMA(schema)
		DIE_schema[1] = schema;
		if (is_debug()) printf("Schema %d: %x (offset %x)\n", 1, schema, schema - elf->section[ELF_SECTION_DEBUG_ABBREV].start);
		nb_types = 1;
		while (schema < elf->section[ELF_SECTION_DEBUG_ABBREV].end) {
			CHECK_VALID_SCHEMA(schema)
			schema += 4;
			while (*schema != 0 || *(schema-1) != 0) schema++;
			schema++;

			if (*schema == 0) break;

			nb_types++;
			if (nb_types >= 100) {
				printf("Error: too many DIE types: %x (%x / %x)\n", nb_types, schema, schema - elf->section[ELF_SECTION_DEBUG_ABBREV].start);
				return;
			}
			DIE_schema[nb_types] = schema;
			if (is_debug()) printf("Schema %d: %x (offset %x)\n", nb_types, schema, schema - elf->section[ELF_SECTION_DEBUG_ABBREV].start);
		}

		uint *tmp = (uint*)die, n1, n2;
		uint compilation_unit_size = *tmp;
		unsigned char *end_compilation_unit = die + header->length + 4;
		uint compilation_unit_start = (uint)die;
		die += 11;
		if (is_debug()) printf("Compilation unit at %x, size: %x\n", (die - start), compilation_unit_size);

		while (die < end_compilation_unit) {
			CHECK_VALID_DIE(die)

			if (*die == 0) {
				die++;
				continue;
			} else if (*die > nb_types) {
				printf("UNKNOWN DIE: %d (%x, offset %x)\n", *die, die, (die - start));
				return;
			}
			if (is_debug()) printf("DIE: %d (%x)\n", *die, (die - start));

			schema = DIE_schema[*die];
			uint8 *schema_top = schema;
			is_function_type = *(schema+1) == DW_TAG_subprogram;
			unsigned char *die_start = die;
/*			if (is_function_type) {
				DWARFSubProgram *subprogram = (DWARFSubProgram*)die;
				printf("[%s] %x -> %x", kernel_debug_str + subprogram->name, subprogram->low_pc, subprogram->low_pc + subprogram->high_pc);
			}*/

			schema += 4;
			CHECK_VALID_SCHEMA(schema)
			die++;

//			printf("Schema: %d/%d (%x)", *schema, *(schema-1), (schema - elf->section[ELF_SECTION_DEBUG_ABBREV].start));

			// Goes through the schema and compute the
			// space taken by each attribute
			while ((*schema != 0 || *(schema-1) != 0)) {
				// The first argument is actually ULEB-encoded.
				// So if it's greater than 0x80 we assume it takes 2 bytes instead of 1
				if (*(schema-1) & 0x80) schema++;

				CHECK_VALID_SCHEMA(schema)
				CHECK_VALID_DIE(die)
				CHECK_VALID_ATTRIBUTE(schema, schema_top)

				if (is_debug()) printf("Attr DW_FORM_%s (%d) (%x, die=%x)\n", DW_FORM_name[*schema], *schema, (schema - elf->section[ELF_SECTION_DEBUG_ABBREV].start), die - elf->section[ELF_SECTION_DEBUG_INFO].start);

				// If this is DIE of type DW_TAG_subprogram
				// we care about its fields
				if (is_function_type) {
					if (*(schema-1) == DW_AT_name || *(schema-1) == DW_AT_linkage_name) {
						if (*schema == DW_FORM_string) functionRanges[nb_function_ranges].name = die;
						else functionRanges[nb_function_ranges].name = elf->section[ELF_SECTION_DEBUG_STR].start + *(uint*)die;
					}
					if (*(schema-1) == DW_AT_specification) functionRanges[nb_function_ranges].address = *(uint*)die;
					if (*(schema-1) == DW_AT_low_pc) functionRanges[nb_function_ranges].low_pc = *(unsigned char **)die;
					if ((uint)*(schema-1) == DW_AT_high_pc) functionRanges[nb_function_ranges].high_pc = *(uint*)die;
				}

				if (DW_FORM_size[*schema] > 0) {
					die += DW_FORM_size[*schema];
				}
				else {
					switch (*schema) {
						case DW_FORM_string:
							die += strlen(die) + 1;
							break;
						case DW_FORM_flag_present:
							break;
						case DW_FORM_exprloc:
							n2 = decodeULEB128(die, &n1);
//							printf("DW_FORM_exprloc [%d] [%d]\n", n1, n2);
//							printf("Old DIE: %d (%x)\n", *die, die);
							die += n1 + n2;
//							printf("New DIE: %d (%x)\n", *die, die);
//							if (*(schema+1) != 0 || *(schema+2) != 0) schema++;
							break;
						default:
							printf("UNKNOWN DW_FORM: %d (%x, offset %x)\n", *schema, schema, schema - elf->section[ELF_SECTION_DEBUG_ABBREV].start);
							return;
					}
				}

				schema += 2;
			}

			if (is_function_type) {
				// The function has no name
				if (functionRanges[nb_function_ranges].name == 0) {
					// If it has a DW_AT_specification, let's check if it was previously recorded
					if (functionRanges[nb_function_ranges].address != 0 &&
						functionRanges[nb_function_ranges].low_pc > 0 &&
						functionRanges[nb_function_ranges].high_pc > 0) {
//						printf("[%x] Searching for address %x...\n", die_start - compilation_unit_start, functionRanges[nb_function_ranges].address);
						for (int i=nb_function_ranges-1; i>=0; i--) {
							if (functionRanges[nb_function_ranges].address == functionRanges[i].address) {
								functionRanges[i].low_pc = functionRanges[nb_function_ranges].low_pc;
								functionRanges[i].high_pc = functionRanges[nb_function_ranges].high_pc;
//								printf("[%s : %x->%x]\n", functionRanges[i].name, functionRanges[i].low_pc, functionRanges[i].low_pc+functionRanges[i].high_pc);
								break;
							}

							if (functionRanges[nb_function_ranges].address <= 11) break;
						}
					}

					functionRanges[nb_function_ranges].address = 0;
					functionRanges[nb_function_ranges].low_pc = 0;
					functionRanges[nb_function_ranges].high_pc = 0;
				}
				// It has a name. Add an entry and unmangle the name if it is a C++ name
				else {
					functionRanges[nb_function_ranges].address = die_start - compilation_unit_start;
					if (!strncmp(functionRanges[nb_function_ranges].name, "_ZN", 3)) {
						cpp_unmanble(functionRanges[nb_function_ranges].name);
//						printf("DIE: %s %d (%x)\n", functionRanges[nb_function_ranges].name, *die_start, (die_start - compilation_unit_start));
//						printf("[%s]\n", functionRanges[nb_function_ranges].name);
					}
					nb_function_ranges++;
				}

			}

			CHECK_VALID_DIE(die)
		}

	}

//	printf("Nb functions: %d\n", nb_function_ranges);
//	return;
//for (;;);
	kheap_check_for_corruption();

/*	for (int i=0; i<nb_function_ranges; i++)
		printf("[%s]\n", functionRanges[i].name);

	printf("------------\n");*/

}
