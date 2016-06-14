#ifndef __DWARF_H
#define __DWARF_H

// This are some includes to uncompress the DWARF
#define DW_TAG_array_type		1
#define DW_TAG_class_type		2
#define DW_TAG_entry_point		3
#define DW_TAG_enumeration_type	4
#define DW_TAG_formal_parameter	5
#define DW_TAG_subprogram		0x2E

#define DW_AT_name			0x3
#define DW_AT_low_pc		0x11
#define DW_AT_high_pc		0x12
#define DW_AT_specification 0x47
#define DW_AT_linkage_name	0x6E

#define DW_FORM_addr		0x1
#define DW_FORM_block2		0x3
#define DW_FORM_block4		0x4
#define DW_FORM_data2		0x5
#define DW_FORM_data4		0x6
#define DW_FORM_data8		0x7
#define DW_FORM_string		0x8
#define DW_FORM_block 		0x9
#define DW_FORM_block1		0xa
#define DW_FORM_data1		0xb
#define DW_FORM_flag		0xc
#define DW_FORM_sdata		0xd
#define DW_FORM_strp		0xe
#define DW_FORM_udata		0xf
#define DW_FORM_ref_addr	0x10
#define DW_FORM_ref1		0x11
#define DW_FORM_ref2		0x12
#define DW_FORM_ref4		0x13
#define DW_FORM_ref8		0x14
#define DW_FORM_ref_udata	0x15
#define DW_FORM_indirect	0x16
#define DW_FORM_sec_offset	0x17
#define DW_FORM_exprloc		0x18
#define DW_FORM_flag_present	0x19
#define DW_FORM_ref_sig8	0x20

/*
extern unsigned char *kernel_debug_info;
extern uint kernel_debug_info_size;
extern unsigned char *kernel_debug_line;
extern uint kernel_debug_line_size;
extern unsigned char *kernel_debug_abbrev;
extern uint kernel_debug_abbrev_size;
extern unsigned char *kernel_debug_str;
*/

int decodeSLEB128(uint8 * leb128, uint *leb128_length);
uint decodeULEB128(uint8 *p, uint *n);

#endif
