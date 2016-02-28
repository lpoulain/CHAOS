#include "libc.h"

int decodeSLEB128(uint8 * leb128, uint *leb128_length)
{
    int number = 0;
    uint8 sign = 0;
    long shift = 0;
    unsigned char byte = *leb128;
    long byte_length = 1;

    /* byte_length being the number of bytes of data absorbed so far in 
       turning the leb into a Dwarf_Signed. */

    for (;;) {
	    sign = byte & 0x40;
	    number |= ((int) ((byte & 0x7f))) << shift;
	    shift += 7;

	    if ((byte & 0x80) == 0) {
	        break;
	    }
	    ++leb128;
	    byte = *leb128;
	    byte_length++;
    }

    if ((shift < sizeof(int) * 8) && sign) {
	    number |= -((int) 1 << shift);
    }

    if (leb128_length != 0)
	*leb128_length = byte_length;
    return (number);
}
/*
int decodeSLEB128(uint8 *p) {
	const uint8 *orig_p = p;
	int Value = 0;
	unsigned Shift = 0;
	uint8 Byte;
	do {
		Byte = *p++;
		Value |= ((Byte & 0x7f) << Shift);
		Shift += 7;
	} while (Byte >= 128);
	// Sign extend negative numbers.
	if (Byte & 0x40)
		Value |= (-1ULL) << Shift;
	return Value;
}*/

uint decodeULEB128(uint8 *p, uint *n) {
	const uint8 *orig_p = p;
	uint Value = 0;
	unsigned Shift = 0;
	do {
		Value += (uint)(*p & 0x7f) << Shift;
		Shift += 7;
	} while (*p++ >= 128);
	if (n)
		*n = (unsigned)(p - orig_p);
	return Value;
}
