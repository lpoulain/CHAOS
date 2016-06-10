extern "C" {
	#include "libc.h"
	#include "kheap.h"
}

#include "tls.hh"

uint8 atoi_hex_c(char *str) {
    uint result = 0, mult = 1;
    int len = 1;
    do {
        if (str[len] >= '0' && str[len] <= '9') result += mult * (str[len] - '0');
        else if (str[len] >= 'A' && str[len] <= 'F') result += mult * (str[len] - 'A' + 10);
        else if (str[len] >= 'a' && str[len] <= 'f') result += mult * (str[len] - 'a' + 10);
        else {
//            debug_i("Unknown character", str[len]);
            return 0;
        }
        len--;
        mult *= 16;
    } while (len >= 0);

    return result;
}


LargeInt::LargeInt(uint16 size) {
	this->size = size;
	data = new uint[size];
	for (int i=0; i<size; i++)
		data[i] = 0;
}

LargeInt::LargeInt(TLSNumber *nb) {
	this->size = nb->size / 4;

	// In case we deal with a very small TLSNumber
	if (this->size == 0) {
		this->size = 1;
		this->data = new uint[1]();
		this->data[0] = 0;
		uint8* tmp = (uint8*)this->data;
		for (int i=0; i<nb->size; i++) {
			tmp[i] = nb->value[nb->size - 1 - i];
		}
		return;
	}

	this->data = new uint[this->size]();

	uint *tmp = (uint*)nb->value;
	for (int i=0; i<this->size; i++) {
		this->data[i] = switch_endian32(tmp[this->size-1-i]);
	}
}

LargeInt::LargeInt(const char hex[]) {
	int size = strlen(hex);
	this->size = (uint16)size/8;
	if (this->size == 0) this->size = 1;
	this->data = new uint[this->size]();

	uint8 *tmp = (uint8*)this->data;
	for (int i=0; i<size/2; i++) {
		tmp[size/2 -1 - i] = atoi_hex_c((char*)hex + i*2);
	}
}

LargeInt::~LargeInt() {
	delete data;
}

void LargeInt::print() {
	uint8 *nb8 = (uint8*)this->data;
	int counter = 0;
	for (int i=this->size-1; i>= 0; i--) {
		printf("%X%X%X%X ", nb8[i*4+3], nb8[i*4+2], nb8[i*4+1], nb8[i*4]);
		if (++counter % 8 == 0) printf("\n");
	}
	printf("\n");
}

int LargeInt::cmp(LargeInt *b) {
	int size = this->size;
	if (this->size != b->size) {
		if (this->size > b->size) {
			for (int i=this->size-1; i>= b->size; i--) {
				if (this->data[i] != 0) return 1;
			}
			size = b->size;
		} else {
			for (int i=b->size-1; i>= this->size; i--) {
				if (b->data[i] != 0) return -1;
			}
			size = this->size;
		}
	}

	for (int i=size-1; i>= 0; i--) {
		if (this->data[i] != b->data[i]) {
			if (this->data[i] > b->data[i]) return 1;
			return -1;
		}
	}

	return 0;
}

bool LargeInt::operator==(LargeInt &a) { return (this->cmp(&a) == 0); }
bool LargeInt::operator>=(LargeInt &a) { return (this->cmp(&a) >= 0); }
bool LargeInt::operator>(LargeInt &a) { return (this->cmp(&a) > 0); }
bool LargeInt::operator<=(LargeInt &a) { return (this->cmp(&a) <= 0); }
bool LargeInt::operator<(LargeInt &a) { return (this->cmp(&a) < 0); }

void LargeInt::operator+=(LargeInt &b) {
	uint64 carry = 0;
	uint *carry32 = (uint*)&carry;
	int size;
	if (this->size > b.size) size = b.size;
	else size = this->size;

	for (int i=0; i< size; i++) {
		carry = (uint64)this->data[i] + (uint64)b.data[i] + carry;
		this->data[i] = carry32[0];
		carry >>= 32;
	}

	if (size+1 <= this->size)
		this->data[size] = carry32[0];
}

void LargeInt::operator-=(LargeInt &b) {
	uint carry = 0;
	for (int i=0; i< this->size; i++) {
		if (this->data[i] < b.data[i] + carry) {
			carry = b.data[i] + carry - this->data[i];
			this->data[i] = (0xFFFFFFFF - carry) + 1;
			carry = 1;
		} else {
			this->data[i] -= (b.data[i] + carry);
			carry = 0;
		}
	}
}

void LargeInt::operator>>=(int nb) {
	if (nb <= 0) return;

	int nb_words = nb/32, high_word=this->size-1;

	if (nb_words > 0) {
		for (int i=0; i<this->size-nb_words; i++)
			this->data[i] = this->data[i+nb_words];
		for (int i=this->size-nb_words; i<this->size; i++)
			this->data[i] = 0;
		nb = nb % 32;
		high_word -= nb_words;
	}

	uint carry = 0, old_carry = 0;
	uint mask1 = 0xFFFFFFFF >> (32-nb);
	
	for (int i=high_word; i>=0; i--) {
		carry = (this->data[i] & mask1) << (32 - nb);
		this->data[i] >>= nb;
		this->data[i] |= old_carry;
		old_carry = carry;
	}
}

void LargeInt::shift_right() {
	int carry = 0, old_carry=0;
	for (int i=this->size-1; i>=0; i--) {
		carry = (this->data[i] & 1);
		this->data[i] >>= 1;
		if (old_carry) this->data[i] |= 0x80000000;
		old_carry = carry;
	}
}

void LargeInt::shift_left() {
	int carry = 0, old_carry=0;
	for (int i=0; i<this->size; i++) {
		carry = (this->data[i] & 0x80000000);
		this->data[i] <<= 1;
		if (old_carry) this->data[i] |= 0x1;
		old_carry = carry;
	}
}

uint LargeInt::nb_top_empty_bits() {
	uint res = 0, cmp, data;

	for (int i=this->size-1; i>=0; i--) {
		data = this->data[i];
		if (data == 0) {
			res += 32;
			continue;
		}

		cmp = 0x80000000;
		for (int j=0; j<32; j++) {
			if (data & cmp) return res;
			cmp >>= 1;
			res ++;
		}
	}

	return res;
}

void LargeInt::modulo(LargeInt &mod) {
	if (mod > *this) return;

	LargeInt mod_large(this->size);

	int bit_shift = 0;

	int this_top_word=this->size-1;
	while (this->data[this_top_word] == 0 && this_top_word > 0) this_top_word--;
	int mod_top_word=mod.size-1;
	while (mod.data[mod_top_word] == 0 && mod_top_word > 0) mod_top_word--;

	for (int i=0; i<=mod_top_word; i++) {
			mod_large.data[this_top_word-mod_top_word+i] = mod.data[i];
	}
	bit_shift += 32*(this_top_word-mod_top_word);

	uint a_top_empty_bits = this->nb_top_empty_bits();
	uint mod_top_empty_bits = mod_large.nb_top_empty_bits();
	uint diff_bits;

//		printf("MODULO %d %d\n", this_top_word, mod_top_word);
//		this->print();
//		mod_large.print();

	while (mod_top_empty_bits > a_top_empty_bits) {
		mod_large.shift_left();
		mod_top_empty_bits--;
		bit_shift++;
	}

	while (bit_shift >= 0) {
		if (this->cmp(&mod_large) >= 0) {
			*this -= mod_large;
		}
		a_top_empty_bits = this->nb_top_empty_bits();
		mod_top_empty_bits = mod_large.nb_top_empty_bits();
		diff_bits = a_top_empty_bits - mod_top_empty_bits;
		if (diff_bits == 0) diff_bits = 1;
//			printf(">> %d (%d %d)\n", diff_bits, a_top_empty_bits, mod_top_empty_bits);
		bit_shift -= diff_bits;
		mod_large >>= diff_bits;
	}
}

void LargeInt::mod_mul(LargeInt *b, LargeInt *mod) {
	LargeInt result(this->size + b->size);
	uint64 carry64=0;
	uint *carry32 = (uint*)&carry64;
    LargeInt intermediate(this->size + b->size);

	for (int b_i=0; b_i < b->size; b_i++) {
		if (b->data[b_i] == 0) continue;
		carry64 = 0;
		for (int i=0; i<b_i; i++) intermediate.data[i] = 0;

		for (int a_i=0; a_i < this->size; a_i++) {
			carry64 += (uint64)this->data[a_i] * (uint64)b->data[b_i];
			intermediate.data[a_i+b_i] = carry32[0];
			carry64 >>= 32;
		}
		intermediate.data[this->size+b_i] = carry32[0];

		result += intermediate;
	}

	result.modulo(*mod);
	for (int i=0; i<this->size; i++) this->data[i] = result.data[i];
}

LargeInt *LargeInt::mod_exp(LargeInt *a, LargeInt *b, LargeInt *mod) {
	LargeInt *res = new LargeInt(mod->size);
	LargeInt large_a(mod->size*2), result(mod->size*2);
	uint pow2;

	for (int i=0; i< a->size; i++) large_a.data[i] = a->data[i];
	result.data[0] = 1;

	for (int i=0; i<b->size; i++) {
		pow2 = 1;
		for (int j=0; j<32; j++) {
			if (b->data[i] & pow2) {
				result.mod_mul(&large_a, mod);
			}

			large_a.mod_mul(&large_a, mod);
			pow2 <<= 1;
		}
	}

	for (int i=0; i<res->size; i++)
		res->data[i] = result.data[i];

	return res;
}

//////////////////////////////////////////////////////////////

TLSNumber::TLSNumber() {
	
}

TLSNumber::TLSNumber(uint16 size, uint8 *value) {
	this->init(size, value);
}

TLSNumber::TLSNumber(uint16 size) {
	this->init(size);
}

TLSNumber::TLSNumber(LargeInt *li) {
	this->init(li->size*4);

	uint *tmp = (uint*)this->value;
	for (int i=0; i<li->size; i++) {
		tmp[i] = switch_endian32(li->data[li->size-1-i]);
	}
}

TLSNumber::~TLSNumber() {
	if (this->own_allocation) delete this->value;
}

void TLSNumber::init(uint16 size, uint8 *value) {
	this->size = size;
	this->value = value;
	this->own_allocation = false;
}

void TLSNumber::init(uint16 size) {
	this->size = size;
	this->value = (uint8*)kmalloc(size);
	this->own_allocation = true;
}

void TLSNumber::print() {
	for (int i=0; i<this->size; i++)
		printf("%X", this->value[i]);
	printf("\n");
}
