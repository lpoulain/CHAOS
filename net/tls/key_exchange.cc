extern "C" {
	#include "libc.h"
	#include "kheap.h"
}

#include "tls.hh"

TLSNumber *KeyExchange::get_premaster_secret() { return 0; }
uint8 *KeyExchange::get_client_key_exchange() { return 0; }
uint16 KeyExchange::get_key_size() { return 0; }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Diffie-Hellman Ephemeral (DHE )key exchange
///////////////////////////////////////////////////////////////////////////////////////////////////

DHE_KeyExchange::DHE_KeyExchange(uint8 *server_key_exchange) {
	// Retrieve the DH parameters p, g and (g^y mod p) from the server key exchange message
	this->server_p.init(server_key_exchange[4] * 256 + server_key_exchange[5],
						server_key_exchange + 6);
	this->server_g.init(*(this->server_p.value + this->server_p.size) * 256 + *(this->server_p.value + this->server_p.size + 1),
					    this->server_p.value + 2 + this->server_p.size);
	this->server_g_y.init(*(this->server_g.value + this->server_g.size) * 256 + *(this->server_g.value + this->server_g.size + 1),
						  this->server_g.value + 2 + this->server_g.size);

	// Because p is going to be used several times, we convert it now to a LargeInt
	server_p_Int = new LargeInt(&this->server_p);
}

uint16 DHE_KeyExchange::get_key_size() {
	return this->server_p.size;
}

TLSNumber *DHE_KeyExchange::get_premaster_secret() {
	uint *tmp;

	// Compute secret x
	this->client_x_Int = new LargeInt("aedebc6285eb3c2a8b949bf3c89d5ab93ef67b13aaa2e6a4b849b48d07889ee7");

	// Convert from TLSNumber to LargeInt
	LargeInt server_g_y_Int(&this->server_g_y);

	// Compute premaster_key = (g^y)^x
	LargeInt *premaster_secret_Int = LargeInt::mod_exp(&server_g_y_Int, this->client_x_Int, this->server_p_Int);

	// Convert premaster_secret_Int (LargeInt) into premaster_secret (TLSNumber)
	TLSNumber *premaster_secret = new TLSNumber(premaster_secret_Int);

	delete premaster_secret_Int;
/*	TLSNumber *premaster_secret = new TLSNumber;
	uint8 *premaster_secret_value = (uint8*)kmalloc(256);
	premaster_secret->value = (uint8*)premaster_secret_value;
	premaster_secret->size = 256;
	tmp = (uint*)premaster_secret_value;
	for (int i=0; i<64; i++) {
		tmp[i] = switch_endian32(premaster_secret_Int->data[63-i]);
	}*/

	return premaster_secret;
}

uint8 *DHE_KeyExchange::get_client_key_exchange() {
	uint8 *client_key_exchange = (uint8*)kmalloc(11 + this->server_p.size);

	LargeInt server_g_Int(&this->server_g);

	// Compute g^x mod p
	LargeInt *client_g_x_Int = LargeInt::mod_exp(&server_g_Int, this->client_x_Int, server_p_Int);

	// Convert g^x from a LargeInt to a TLSNumber
	// stored in the client_key_exchange message
	uint16 *tmp16 = (uint16*)(client_key_exchange + 9);
	*tmp16 = switch_endian16(this->server_p.size);
	uint *tmp = (uint*)(client_key_exchange + 11);
	for (int i=0; i<64; i++) {
		tmp[i] = switch_endian32(client_g_x_Int->data[63-i]);
	}

	return client_key_exchange;
}

DHE_KeyExchange::~DHE_KeyExchange() {
	delete this->client_x_Int;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// RSA key exchange
///////////////////////////////////////////////////////////////////////////////////////////////////

RSA_KeyExchange::RSA_KeyExchange(uint8 *certificate) {
	ASN1 cert(certificate+10);
	cert.child(0);
	cert.child(6);
	cert.child(1);
	cert.child(0);
	cert.child(0);

	this->RSA_n = cert.get_number();
	this->RSA_e = cert.get_number();
}

uint16 RSA_KeyExchange::get_key_size() {
	return this->RSA_n->size;
}

TLSNumber *RSA_KeyExchange::get_premaster_secret() {
//		self.premaster_secret = TLS_VERSION + os.urandom(46)
//		return bytes_to_int(self.premaster_secret)
	this->premaster_data = (uint8*)kmalloc(256);
	TLSNumber *premaster_secret = new TLSNumber(48, this->premaster_data + 256 - 48);
//	premaster_secret->size = 256;
//	premaster_secret->value = this->premaster_data + 256 - 48;
	premaster_secret->value[0] = 0x03;
	premaster_secret->value[1] = 0x03;

	for (int i=2; i<48; i++) {
		premaster_secret->value[i] = rand() % 256;
	}

	return premaster_secret;
}

uint8 *RSA_KeyExchange::get_client_key_exchange() {
//		premaster_secret = b'\x00\x02' + b'\x42' * (256 - 3 - len(self.premaster_secret)) + b'\x00' + self.premaster_secret
//		
//		encrypted_premaster_secret = pow(bytes_to_int(premaster_secret), self.certificate.RSA_e, self.certificate.RSA_n)
//		msg = hex_to_bytes('100001020100') + nb_to_bytes(encrypted_premaster_secret)
//		return msg
	TLSNumber nb(256, this->premaster_data);
//	nb.size = 256;
//	nb.value = this->premaster_data;
	nb.value[0] = 0x00;
	nb.value[1] = 0x02;
	for (int i=2; i<256-49; i++) nb.value[i] = 0x42;
	nb.value[256-49] = 0;

	LargeInt premaster_secret_Int(&nb), RSA_e_int(this->RSA_e), RSA_n_int(this->RSA_n);
	LargeInt *encrypted_premaster_secret = LargeInt::mod_exp(&premaster_secret_Int, &RSA_e_int, &RSA_n_int);

	uint8 *client_key_exchange = (uint8*)kmalloc(11 + this->RSA_n->size);

	uint16 *tmp16 = (uint16*)(client_key_exchange + 9);
	*tmp16 = switch_endian16(this->RSA_n->size);
	uint *tmp = (uint*)(client_key_exchange + 11);
	for (int i=0; i<64; i++) {
		tmp[i] = switch_endian32(encrypted_premaster_secret->data[63-i]);
	}

	delete encrypted_premaster_secret;
	delete this->premaster_data;

	return client_key_exchange;
}

RSA_KeyExchange::~RSA_KeyExchange() {
	delete this->RSA_n;
	delete this->RSA_e;
}
