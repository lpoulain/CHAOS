extern "C" {
	#include "libc.h"
}

#include "tls.hh"

ASN1::ASN1(uint8 *content) {
	this->cursor = content;
}

uint ASN1::get_size() {
	uint size = (uint)*(this->cursor++), new_size;
	if (size > 128) {
		size -= 128;

		new_size = 0;
		uint exp=1;
		for (int i=size-1; i>=0; i--) {
			new_size += ((uint)*(this->cursor + i)) * exp;
			exp *= 256;
		}

		this->cursor += size;
		size = new_size;
	}
}

void ASN1::next() {		
	uint8 content_type = *(this->cursor++);

	if (content_type == 0x05) {
		this->cursor++;
		return;
	}

	uint size = this->get_size();
	this->cursor += size;
}

void ASN1::child(uint nb) {
	uint8 content_type = *(this->cursor++);
//		printf("[%X] ", content_type);

	if (nb > 0 && content_type != 0x30 && content_type != 0x31) return;

	this->get_size();

	if (content_type == 0x03 || content_type == 0x13 || content_type == 0xa3) {
		if (*(this->cursor) == 0) this->cursor++;
	}

	for (int i=0; i<nb; i++) this->next();
}

TLSNumber *ASN1::get_number() {
	uint8 content_type = *(this->cursor++);
	uint size = this->get_size();

	if (*(this->cursor) == 0) {
		this->cursor++;
		size--;
	}

	TLSNumber *nb = new TLSNumber((uint16)size, this->cursor);

	this->cursor += size;

	return nb;
}
