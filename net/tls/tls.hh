#ifndef __TLS_HH
#define __TLS_HH

/*
typedef struct __attribute__((packed)) {
	uint16 size;
	uint8 *value;
} TLSNumber;
*/

class LargeInt;

class TLSNumber {
public:
	uint16 size;
	uint8 *value;
	uint8 own_allocation;

	TLSNumber();
	TLSNumber(uint16, uint8 *);
	TLSNumber(uint16);
	TLSNumber(LargeInt *);
	~TLSNumber();
	void init(uint16, uint8 *);
	void init(uint16);
	void print();
};

class LargeInt {
public:
	uint16 size;
	uint *data;

	LargeInt(uint16 size);
	LargeInt(TLSNumber *nb);
	LargeInt(const char hex[]);
	~LargeInt();
	void print();
	int cmp(LargeInt *b);
	bool operator==(LargeInt &a);
	bool operator>=(LargeInt &a);
	bool operator>(LargeInt &a);
	bool operator<=(LargeInt &a);
	bool operator<(LargeInt &a);
	void operator+=(LargeInt &b);
	void operator-=(LargeInt &b);
	void shift_right();
	void shift_left();
	void operator>>=(int);
	uint nb_top_empty_bits();
	void modulo(LargeInt &mod);
	void mod_mul(LargeInt *b, LargeInt *mod);
	static LargeInt *mod_exp(LargeInt *a, LargeInt *b, LargeInt *mod);
};

class ASN1 {
	uint8 *cursor;
public:
	ASN1(uint8 *content);
	uint get_size();
	void next();
	void child(uint);
	TLSNumber *get_number();
};

////////////////////////////////////////////////////

class KeyExchange {
public:
	virtual TLSNumber *get_premaster_secret();
	virtual uint8 *get_client_key_exchange();
	virtual uint16 get_key_size();
};

class DHE_KeyExchange : public KeyExchange {
	TLSNumber server_p;
	TLSNumber server_g;
	TLSNumber server_g_y;
	LargeInt *server_p_Int;
	LargeInt *client_x_Int;

public:
	DHE_KeyExchange(uint8 *);
	virtual uint16 get_key_size();
	virtual TLSNumber *get_premaster_secret();
	virtual uint8 *get_client_key_exchange();
};

class RSA_KeyExchange : public KeyExchange {
	TLSNumber *RSA_n;
	TLSNumber *RSA_e;
	uint8 *premaster_data;

public:
	RSA_KeyExchange(uint8 *);
	virtual uint16 get_key_size();
	virtual TLSNumber *get_premaster_secret();
	virtual uint8 *get_client_key_exchange();
};

#endif
