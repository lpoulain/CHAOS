extern "C" {
	#include "libc.h"
	#include "display.h"
	#include "tcp.h"
	#include "kheap.h"
	#include "crypto.h"
}

#include "tls.hh"

#define TLS_VERSION							0x0303
#define TLS_CHANGE_CIPHER_SPEC				0x14
#define TLS_ALERT 							0x15
#define TLS_HANDSHAKE						0x16
#define TLS_APPLICATION_DATA				0x17
#define TLS_HANDSHAKE_CLIENT_HELLO			0x01
#define TLS_HANDSHAKE_SERVER_HELLO			0x02
#define TLS_HANDSHAKE_CERTIFICATE			0x0B
#define TLS_HANDSHAKE_SERVER_KEY_EXCHANGE	0x0C
#define TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE	0x10
#define TLS_HANDSHAKE_SERVER_HELLO_DONE		0x0E

#define TLS_DHE_RSA_WITH_AES_128_CBC_SHA	0x3300
#define TLS_RSA_WITH_AES_128_CBC_SHA 		0x2F00

typedef struct __attribute__((packed)) {
	uint8 content_type;
	uint16 version;
	uint16 length;
} TLSRecord;

typedef struct __attribute__((packed)) {
	uint8 handshake_type;
	uint8 padding;	// The TLSRecord length is only 16-bit, so not sure why the TLSHandshake is 24-bit
	uint16 length;
	uint16 version;
} TLSHandshake;

typedef struct {
	TCPData **data;
	uint offset;
} TLSCursor;

void TLSCursor_init(TLSCursor *cursor, TCPConnection *connection) {
	cursor->data = &(connection->data_first);
	cursor->offset = 0;
}

uint8 *TLSCursor_next(TLSCursor *cursor, uint nb_bytes) {
	while (*cursor->data == 0);

	while (cursor->offset + nb_bytes > (*(cursor->data))->size) {
//		printf("%d / %d\n", cursor->offset + nb_bytes , (*(cursor->data))->size);
		nb_bytes = nb_bytes - (*(cursor->data))->size + cursor->offset;
//		printf("New offset: %d\n", offset);
		cursor->data = &(*(cursor->data))->next;
		while (*cursor->data == 0);
		cursor->offset = 0;
	}

	cursor->offset += nb_bytes;

	return (uint8*)(*(cursor->data))->content + cursor->offset;
}

uint8 *TLSCursor_copy_next(TLSCursor *cursor, uint nb_bytes, uint8 *buffer) {
	while (*cursor->data == 0);
	uint buffer_offset = 0, bytes_left_in_data;

	while (cursor->offset + nb_bytes > (*(cursor->data))->size) {
		bytes_left_in_data = (*(cursor->data))->size - cursor->offset;
		nb_bytes -= bytes_left_in_data;
		memcpy(buffer + buffer_offset, (*(cursor->data))->content + cursor->offset, bytes_left_in_data);
//		printf("Copy %d bytes (%x -> %x)\n", bytes_left_in_data, buffer + buffer_offset, buffer + buffer_offset + bytes_left_in_data);
		buffer_offset += bytes_left_in_data;

//		printf("New offset: %d\n", offset);
		cursor->data = &(*(cursor->data))->next;
		while (*cursor->data == 0);
		cursor->offset = 0;
	}

	memcpy(buffer + buffer_offset, (*(cursor->data))->content + cursor->offset, nb_bytes);
//	printf("Copy %d bytes (%x -> %x)\n", nb_bytes, buffer + buffer_offset, buffer + buffer_offset + nb_bytes);
	cursor->offset += nb_bytes;

	return (uint8*)(*(cursor->data))->content + cursor->offset;
}

uint8 TLSCursor_next_byte(TLSCursor *cursor) {
	uint8 *tmp = TLSCursor_next(cursor, 1);
	return *tmp;
}

uint8 *TLSCursor_current(TLSCursor *cursor) {
	while (*cursor->data == 0);
	return (*(cursor->data))->content + cursor->offset;
}


/////////////////////////////////////////////////////

typedef struct {
	TLSNumber plaintext;	// Contains the plaintext + other data
	uint16 plaintext_size;	// The official plaintext size
	TLSNumber ciphertext;	// The ciphertext as sent through TCP/IP
} TLSEncryptedMessage;

TLSEncryptedMessage *message_new(uint8 content_type, uint16 size) {
	TLSEncryptedMessage *msg = (TLSEncryptedMessage*)kmalloc(sizeof(TLSEncryptedMessage));

	msg->plaintext.init(13 + 16 * ((size + 20) / 16) + 16);			// 13 bytes are used to compute the MAC
																	// size + 20: plaintext + MAC
																	// (size + 20) / 16 + 16: plaintext + MAC + CBC padding
	msg->plaintext_size = size;

	msg->ciphertext.size = 16 * ((size + 20) / 16) + 32 + 5;					// IV + plaintext + MAC + CBC padding
	msg->ciphertext.value = (uint8*)kmalloc(5 + msg->ciphertext.size);					// 5 bytes for the TLS header
	msg->ciphertext.value[0] = content_type;
	msg->ciphertext.value[1] = 0x03;
	msg->ciphertext.value[2] = 0x03;
	msg->ciphertext.value[3] = (uint8)((msg->ciphertext.size - 5) >> 8) & 0xFF;
	msg->ciphertext.value[4] = (uint8)((msg->ciphertext.size - 5) & 0xFF);

	return msg;
}

uint8 *message_load(uint8 *ciphertext, uint16 size) {
	TLSEncryptedMessage *msg = (TLSEncryptedMessage*)kmalloc(sizeof(TLSEncryptedMessage));

	msg->ciphertext.init(size, ciphertext);

	msg->plaintext.init(size - 16 + 13);		// The plaintext doesn't contain the IV (16 bytes)
												// But for compatibility we need the 13-bytes overhead
//	msg->plaintext.value = (uint8*)kmalloc(msg->plaintext.size);
	msg->plaintext_size = size - 16;

	return (uint8*)msg;
}

uint8 *message_plaintext(TLSEncryptedMessage *msg) {
	return msg->plaintext.value + 13;
}

uint8 *message_ciphertext(TLSEncryptedMessage *msg) {
	return msg->ciphertext.value + 5;
}

uint8 *message_all(TLSEncryptedMessage *msg) {
	return msg->ciphertext.value;
}

void message_free(TLSEncryptedMessage *msg) {
//	kfree(msg->plaintext.value);
//	kfree(msg->ciphertext.value);
	kfree(msg);
}

// Ciphertext: IV | AES(16-bytes plaintext | 20-bytes HMAC+SHA1 | 12-bytes padding)
void message_encrypt(TLSEncryptedMessage *msg, TLSNumber *AES_key, TLSNumber *MAC_key, uint seq_num, uint8 content_type) {
	uint8 iv[16];
	for (int i=0; i<32; i++) {
		iv[i] = rand() % 256;
	}

	// Writes the first 13 bytes, which are used to compute the MAC
	uint *tmp32 = (uint*)msg->plaintext.value;
	*tmp32++ = 0;
	*tmp32 = switch_endian32(seq_num);
	uint8 *tmp8 = msg->plaintext.value + 8;
	*tmp8++ = content_type;
	*tmp8++ = 0x03;
	*tmp8++ = 0x03;
	*tmp8++ = (uint8)((msg->plaintext_size) >> 8);
	*tmp8++ = (uint8)((msg->plaintext_size) & 0xFF);

	// Computes the MAC and write it after the plaintext
	// (TLS is using a MAC-then-Encrypt approach)
	HMAC_SHA1(msg->plaintext.value, 13 + msg->plaintext_size, MAC_key->value, MAC_key->size, msg->plaintext.value + 13 + msg->plaintext_size);

	// Writes the CBC padding
	uint8 pad = 15 - (msg->plaintext_size + 20) % 16;
	for (int i=msg->plaintext.size - pad - 1; i<msg->plaintext.size; i++) {
		msg->plaintext.value[i] = pad;
	}

	// Writes the IV in the ciphertext
	uint8 *ciphertext = message_ciphertext(msg);
	memcpy(ciphertext, iv, 16);

	// Encrypt the plaintext + MAC
	aes_encrypt_cbc(msg->plaintext.value + 13, 16 * ((msg->plaintext_size + 20) / 16 + 1), ciphertext + 16, AES_key->value, AES_key->size*8, iv);
}

void message_decrypt(TLSEncryptedMessage *msg, TLSNumber *AES_key, TLSNumber *MAC_key, uint seq_num, uint8 content_type) {
	aes_decrypt_cbc(msg->ciphertext.value + 16, msg->ciphertext.size - 16, message_plaintext(msg), AES_key->value, AES_key->size*8, msg->ciphertext.value);
	msg->plaintext_size -= (msg->plaintext.value[msg->plaintext.size-1] + 1);	// Remove the CBC padding
//	printf("Padding: %d => Size=%d\n", (msg->plaintext.value[msg->plaintext.size-1] + 1), msg->plaintext_size);
	msg->plaintext_size -= 20;		// Remove the MAC
}

uint8 TLS_debug = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TLS {
	TCPConnection *connection;
	TLSCursor cursor;
	KeyExchange *key_exchange;
	uint handshake_size;

	TLSNumber client_random;
	TLSNumber server_random;

	uint8 *client_hello;
	uint8 *server_hello;
	uint8 *server_certificate;
	uint8 *server_key_exchange;
	uint8 *server_hello_done;
	uint8 *client_key_exchange;

	uint8 *handshake_buffer;

	uint16 key_size;

	TLSNumber client_write_key;
	TLSNumber server_write_key;
	TLSNumber client_write_MAC_key;
	TLSNumber server_write_MAC_key;

	TLSNumber master_secret;

public:
	TLS(Window *win, uint ip, char *hostname, uint8 payload[]) {
		this->send_client_hello(ip, hostname);
		printf_win(win, ".");
		if (this->receive_server_hello(win) < 0) return;
		printf_win(win, ".");
		this->compute_secret_keys();
		printf_win(win, ".");
		this->send_client_key_exchange();
		printf_win(win, ".");
		this->send_client_change_cipher_suite();
		printf_win(win, ".");
		this->send_client_encrypted_handshake();
		printf_win(win, ".");
		if (this->receive_server_end_handshake(win) < 0) return;
		printf_win(win, ".");
		this->sends_GET_request(hostname, payload);
		printf_win(win, ".\n");
		this->receive_HTML_response(win);
	}

	void HMAC_hash(TLSNumber *key, TLSNumber *secret, uint8 out[]) {
		HMAC_SHA256(secret->value, secret->size, key->value, key->size, out);
	}

	// Returns a result whose size is a multiple of 32 bytes
	void P_hash(TLSNumber *secret, TLSNumber *seed, uint8 out[], uint output_size) {
		// A (initially 'seed', but after the first iteration will always be 32 bytes)
		TLSNumber A(seed->size, seed->value);
//		A.value = seed->value;
//		A.size = seed->size;
		uint8 A_data[32];

		// A + seed (always 32+seed's size bytes)
		TLSNumber A_plus_seed(seed->size+32);
//		uint8 *A_plus_seed_value = (uint8*)kmalloc(seed->size + 32);
//		A_plus_seed.value = A_plus_seed_value;
//		A_plus_seed.size = seed->size + 32;

		uint8 hash[32];
		uint result_offset = 0;

		while (output_size > 0) {
			// A = HMAD_hash(secret, A)
			HMAC_hash(secret, &A, hash);
			A.init(32, (uint8*)&A_data);
//			A.value = (uint8*)&A_data;
//			A.size = 32;
			memcpy(A.value, hash, 32);

			// result += HMAC_hash(secret, A + seed)
			memcpy(A_plus_seed.value, A.value, A.size);
			memcpy(A_plus_seed.value + A.size, seed->value, seed->size);
			HMAC_hash(secret, &A_plus_seed, hash);
			memcpy(out + result_offset, hash, 32);

			// output_size -= 32
			result_offset += 32;
			output_size -= 32;
		}
	}

	void PRF(TLSNumber *secret, const char *label, TLSNumber *seed1, TLSNumber *seed2, uint8 out[], uint output_size) {
		uint label_size = strlen(label);
		uint seed_size = label_size + seed1->size;
		if (seed2 != 0) seed_size += seed2->size;
		TLSNumber new_seed(seed_size);

//		new_seed.value = (uint8*)kmalloc(new_seed.size);
		memcpy(new_seed.value, label, label_size);
		memcpy(new_seed.value + label_size, seed1->value, seed1->size);
		if (seed2 != 0) memcpy(new_seed.value + label_size + seed1->size, seed2->value, seed2->size);

		P_hash(secret, &new_seed, out, output_size);

//		kfree(new_seed.value);
	}

	void handle_alert(Window *win, TLSCursor *cursor) {
		uint8 *alert_code = TLSCursor_next(cursor, 1);
		printf_win(win, "\nAlert %d: ", *alert_code);

		switch(*alert_code) {
		case 0:
			printf_win(win, "close_notify"); break;
		case 10:
			printf_win(win, "unexpected_message"); break;
		case 20:
			printf_win(win, "bad_record_mac"); break;
		case 21:
			printf_win(win, "decryption_failed"); break;
		case 22:
			printf_win(win, "record_overflow"); break;
		case 30:
			printf_win(win, "decompression_failure"); break;
		case 40:
			printf_win(win, "handshake_failure"); break;
		case 41:
			printf_win(win, "no_certificate"); break;
		case 42:
			printf_win(win, "bad_certificate"); break;
		case 43:
			printf_win(win, "unsupported_certificate"); break;
		case 44:
			printf_win(win, "certificate_revoked"); break;
		case 45:
			printf_win(win, "certificate_expired"); break;
		case 46:
			printf_win(win, "certificate_unknown"); break;
		case 47:
			printf_win(win, "illegal_parameter"); break;
		case 48:
			printf_win(win, "unknown_ca"); break;
		case 49:
			printf_win(win, "access_denied"); break;
		case 50:
			printf_win(win, "decrypt_error"); break;
		case 51:
			printf_win(win, "decrypt_error"); break;
		case 60:
			printf_win(win, "export_restriction"); break;
		case 70:
			printf_win(win, "protocol_version"); break;
		case 71:
			printf_win(win, "insufficient_security"); break;
		case 80:
			printf_win(win, "internal_error"); break;
		case 90:
			printf_win(win, "user_canceled"); break;
		case 100:
			printf_win(win, "no_renegotiation"); break;
		case 255:
			printf_win(win, "unsupported_extension"); break;
		}

		printf_win(win, "\n");
	}

	uint8* TLS_record(uint8 content_type, uint8 *message) {
		return 0;
	}

	void send_client_hello(uint ip, char *hostname) {
		uint16 hostname_length = (uint16)strlen(hostname), offset;
		this->client_hello = (uint8*)kmalloc(517);
		memset(this->client_hello, 0, 517);

		// TLS Record header
		TLSRecord *record = (TLSRecord*)this->client_hello;
		record->content_type = TLS_HANDSHAKE;
		record->version = TLS_VERSION;
		record->length = switch_endian16(512);

		// TLS Handshake header
		TLSHandshake *handshake = (TLSHandshake*)(this->client_hello + sizeof(TLSRecord));
		handshake->handshake_type = TLS_HANDSHAKE_CLIENT_HELLO;
		handshake->length = switch_endian16(508);
		handshake->version = TLS_VERSION;

		// Client random data
		this->client_random.init(32, this->client_hello + 11);

		for (int i=0; i<32; i++) {
			this->client_random.value[i] = rand() % 256;
		}

		this->client_hello[45] = 0x02;	// 1 supported cipher suite
		this->client_hello[47] = 0x33;	// supported cipher suite: TLS_DHE_RSA_WITH_AES_128_CBC_SHA
		this->client_hello[47] = 0x2F;	// supported cipher suite: TLS_RSA_WITH_AES_128_CBC_SHA
		this->client_hello[48] = 0x01;	// 1 compression method (null)

		// TLS Extensions
		uint16 *tmp16 = (uint16*)(this->client_hello + 50);
		*tmp16++ = switch_endian16(512 - 47);	// Extensions length
		*tmp16++ = 0x01FF;						// renegotiation_info
		*tmp16++ = 0x0100;						// length=1

		// server_name extension
		tmp16 = (uint16*)(this->client_hello + 59);
		*tmp16++ = switch_endian16(hostname_length + 5);	// length
		*tmp16++ = switch_endian16(hostname_length + 3);	// length (again)
		tmp16 = (uint16*)(this->client_hello + 64);
		*tmp16 = switch_endian16(hostname_length);		// actual length
		strncpy((char*)this->client_hello + 66, hostname, hostname_length);	// hostname

		// signature_algorithms extension
		offset = 66 + hostname_length;
		tmp16 = (uint16*)(this->client_hello + offset);
		*tmp16++ = 0x0D00;	// signature_algorithms
		*tmp16++ = 0x1200;	// length=18
		*tmp16++ = 0x1000;	// signature hash algo length=16
		*tmp16++ = 0x0106;	// SHA512 + RSA
		*tmp16++ = 0x0306;	// SHA512 + ECDSA
		*tmp16++ = 0x0105;	// SHA384 + RSA
		*tmp16++ = 0x0305;	// SHA384 + ECDSA
		*tmp16++ = 0x0104;	// SHA256 + RSA
		*tmp16++ = 0x0304;	// SHA256 + ECDSA
		*tmp16++ = 0x0102;	// SHA1 + RSA
		*tmp16++ = 0x0302;	// SHA1 + ECDSA
		offset += 22;

		// ec_point_formats extension
		*tmp16++ = 0x0B00;	// ec_point_formats
		*tmp16++ = 0x0200;	// length=2
		*tmp16++ = 0x0001;	// EC point formats length=1 (uncompressed)
		offset += 6;

		// elliptic curves extension
		*tmp16++ = 0x0A00;	// elliptic_curves
		*tmp16++ = 0x0600;	// length=6
		*tmp16++ = 0x0400;	// elliptic curves length=4
		*tmp16++ = 0x1700;	// secp256r1
		*tmp16++ = 0x1800;	// secp384r1
		offset += 10;

		// padding extension
		*tmp16++ = 0x1500;	// padding
		*tmp16++ = switch_endian16(512 - offset + 1);	// length

		// We will need to copy the complete handshake in a single allocation
		// to compute the MAC. We thus need to compute its size
		// We first add the Client Hello message size
		this->handshake_size = 512;

		this->connection = TCP_start_connection(ip, TCP_PORT_HTTPS, this->client_hello, 517);
	}

	int receive_server_hello(Window *win) {
		TLSCursor_init(&this->cursor, this->connection);
		uint8 keep_downloading = 1;
		uint16 size;

		while (keep_downloading) {
			TLSRecord *record = (TLSRecord *)TLSCursor_current(&this->cursor);
			size = switch_endian16(record->length);
	//		printf("Content type: %X, size:%d\n", record->content_type, size);
			uint8 *tmp = TLSCursor_next(&this->cursor, 5);
	//		printf("Content subtype: %X\n", tmp[0]);
			this->handshake_size += size;

			if (tmp[0] == TLS_HANDSHAKE_SERVER_HELLO_DONE) {
				keep_downloading = 0;
	//			break;
			}
			else TLSCursor_next(&this->cursor, size);
		};

		// Allocates the handshake_buffer, which concatenates all the handshake
		// messages
		this->handshake_size += 64;
		this->handshake_size += 14;
		this->handshake_size += 5 + 256;	// Client Key Exchange

		this->handshake_buffer = (uint8*)kmalloc(this->handshake_size);
	//	printf("Allocating %d bytes\n", handshake_size);
	//	printf("Handshake: %x->%x\n", handshake_buffer, handshake_buffer+handshake_size);
		memcpy(this->handshake_buffer, this->client_hello + 5, 512);
		this->handshake_size = 512;

		// Second pass: we go through to copy

		uint8 *server_hello, *server_certificate, *server_key_exchange, *server_hello_done;
		TLSCursor_init(&this->cursor, this->connection);
		keep_downloading = 1;

		while (keep_downloading) {
			TLSRecord *record = (TLSRecord *)TLSCursor_current(&this->cursor);
			size = switch_endian16(record->length);
	//		printf("Content type: %X, size:%d\n", record->content_type, size);
			uint8 *tmp = TLSCursor_next(&this->cursor, 5);
	//		printf("Content subtype: %X\n", tmp[0]);

			switch(tmp[0]) {
				case TLS_HANDSHAKE_SERVER_HELLO:
					this->server_hello = this->handshake_buffer + this->handshake_size;
					break;
				case TLS_HANDSHAKE_CERTIFICATE:
					this->server_certificate = this->handshake_buffer + this->handshake_size;
					break;
				case TLS_HANDSHAKE_SERVER_KEY_EXCHANGE:
					this->server_key_exchange = this->handshake_buffer + this->handshake_size;
					break;
				case TLS_HANDSHAKE_SERVER_HELLO_DONE:
					this->server_hello_done = this->handshake_buffer + this->handshake_size;
					keep_downloading = 0;
					break;
			}

			TLSCursor_copy_next(&this->cursor, size, this->handshake_buffer + this->handshake_size);
			this->handshake_size += size;
		};

		// Copy the Server Hello Done message
		memcpy(this->handshake_buffer + this->handshake_size - 4, this->server_hello_done, 4);

	//	dump_mem(server_key_exchange, 300, 14);

		// Receive the server random
		this->server_random.init(32, this->server_hello + 6);

		// Call the key exchange (not hard-coded to DHE-RSA)
		// This will decrypt the Diffie-Hellman parameters
		uint16 *cipher_suite = (uint16*)(this->server_hello + 39 + (this->server_hello[38]));

		switch(*cipher_suite) {
			case TLS_DHE_RSA_WITH_AES_128_CBC_SHA:
				printf_win(win, "TLS_DHE_RSA_WITH_AES_128_CBC_SHA");
				this->key_exchange = new DHE_KeyExchange(this->server_key_exchange);
				break;
			case TLS_RSA_WITH_AES_128_CBC_SHA:
				printf_win(win, "TLS_RSA_WITH_AES_128_CBC_SHA");
				this->key_exchange = new RSA_KeyExchange(this->server_certificate);
				break;
			default:
				uint8 *tmp = (uint8*)cipher_suite;
				printf_win(win, "Unknown cipher suite: %X%X\n", tmp[0], tmp[1]);
				return -1;
		}

		// We get the key size used for
		// - premaster_secret size
		// - size of the 
		this->key_size = this->key_exchange->get_key_size();
		return 0;
	}

	void compute_secret_keys() {
		TLSNumber *premaster_secret = this->key_exchange->get_premaster_secret();
		this->master_secret.init(48, (uint8*)kmalloc(64));
//		this->master_secret.size = 48;

		PRF(premaster_secret, "master secret", &this->client_random, &this->server_random, this->master_secret.value, 64);

		uint8 *keys = (uint8*)kmalloc(128);

		PRF(&this->master_secret, "key expansion", &this->server_random, &this->client_random, keys, 128);

		this->client_write_MAC_key.init(20, keys);
		this->server_write_MAC_key.init(20, keys+20);
		this->client_write_key.init(16, keys+40);
		this->server_write_key.init(16, keys+56);
/*
		this->client_write_MAC_key.value = keys;
		this->client_write_MAC_key.size = 20;

		this->server_write_MAC_key.value = keys+20;
		this->server_write_MAC_key.size = 20;

		this->client_write_key.value = keys+40;
		this->client_write_key.size = 16;

		this->server_write_key.value = keys+56;
		this->server_write_key.size = 16;
		*/
	}

	void send_client_key_exchange() {
		// TLS Record header
		this->client_key_exchange = this->key_exchange->get_client_key_exchange();
		TLSRecord *record = (TLSRecord*)client_key_exchange;
		record->content_type = TLS_HANDSHAKE;
		record->version = TLS_VERSION;
		record->length = switch_endian16(6 + this->key_size);

		// TLS Handshake header
		TLSHandshake *handshake = (TLSHandshake*)(this->client_key_exchange + sizeof(TLSRecord));
		handshake->handshake_type = TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE;
		handshake->padding = 0;
		handshake->length = switch_endian16(this->key_size + 2);

		// Append the message to handshake_buffer
		memcpy(this->handshake_buffer + this->handshake_size, this->client_key_exchange + 5, 6 + this->key_size);
		this->handshake_size += 6 + this->key_size;

		TCP_send(client_key_exchange, 11 + this->key_size);
	}

	void send_client_change_cipher_suite() {
		uint8 client_change_cipher_spec[6] = { 0x14, 0x03, 0x03, 0x00, 0x01, 0x01 };
		TCP_send(client_change_cipher_spec, 6);
	}

	void send_client_encrypted_handshake() {
		TLSEncryptedMessage *msg = message_new(TLS_HANDSHAKE, 16);

		uint8* plaintext = message_plaintext(msg);
		plaintext[0] = 0x14;
		plaintext[1] = 0x00;
		plaintext[2] = 0x00;
		plaintext[3] = 0x0C;

		// Computes the hash of the handshake messages
		TLSNumber handshake_msg_hash(32);
//		uint8 handshake_msg_hash_data[32];
//		handshake_msg_hash.value = (uint8*)&handshake_msg_hash_data;
//		handshake_msg_hash.size = 32;
		SHA256(this->handshake_buffer, this->handshake_size, handshake_msg_hash.value);

		// The PRF will overwrite beyond the 12 bytes we want, but the MAC will be computed afterwards
	//	printf("msg=%x, hash=>%x\n", &plaintext_data, (uint8*)(plaintext.value) + 13 + 4);
		this->PRF(&this->master_secret, "client finished", &handshake_msg_hash, 0, plaintext + 4, 32);

	//	printf("\n%x %x\n\n", &client_encrypted_handshake_message, ciphertext.value);
		message_encrypt(msg, &this->client_write_key, &this->client_write_MAC_key, 0, TLS_HANDSHAKE);

		TCP_cleanup_connection();
		TLSCursor_init(&this->cursor, this->connection);

		TCP_send(message_all(msg), msg->ciphertext.size);
		message_free(msg);
	}

	int receive_server_end_handshake(Window *win) {
		uint8 keep_downloading = 1;

		while (keep_downloading) {
			TLSRecord *record = (TLSRecord *)TLSCursor_current(&this->cursor);
			uint16 size = switch_endian16(record->length);
	//		printf("Content type: %X, size:%d\n", record->content_type, size);
			uint8 *tmp = TLSCursor_next(&this->cursor, 5);
	//		printf("Content subtype: %X\n", tmp[0]);

			if (record->content_type == TLS_ALERT) {
				this->handle_alert(win, &this->cursor);
				return -1;
			}
			if (record->content_type == TLS_CHANGE_CIPHER_SPEC) keep_downloading = 0;

			TLSCursor_next(&this->cursor, size);
		};

		TCP_cleanup_connection();
		TLSCursor_init(&this->cursor, this->connection);
		return 0;
	}

	void sends_GET_request(char *hostname, uint8 payload[]) {
		uint16 payload_size = strlen((const char*)payload);
	//	uint16 hostname_size = strlen(hostname), payload_size = ;
		TLSEncryptedMessage *msg = message_new(TLS_APPLICATION_DATA, payload_size);

		uint8* plaintext = message_plaintext(msg);
		memcpy(plaintext, payload, payload_size);
	/*	memcpy(plaintext, "GET / HTTP/1.1\r\nHOST: ", 22);
		memcpy(plaintext+22, hostname, hostname_size);
		memcpy(plaintext+22+hostname_size, "\r\n\r\n", 4);*/

		message_encrypt(msg, &this->client_write_key, &this->client_write_MAC_key, 1, TLS_APPLICATION_DATA);
		TCP_send(message_all(msg), msg->ciphertext.size);

		message_free(msg);
	}

	void receive_HTML_response(Window *win) {
		uint8 keep_downloading = 1;

		while (keep_downloading) {
			TLSRecord *record = (TLSRecord *)TLSCursor_current(&this->cursor);
			uint16 size = switch_endian16(record->length);
	//		printf("Content type: %X, size:%d\n", record->content_type, size);
			uint8 *tmp = TLSCursor_next(&this->cursor, 5);
	//		printf("Content subtype: %X\n", tmp[0]);

			if (record->content_type == TLS_CHANGE_CIPHER_SPEC) keep_downloading = 0;

			uint8 *data = (uint8*)kmalloc(size);
			TLSCursor_copy_next(&this->cursor, size, data);
			TLSEncryptedMessage *msg = (TLSEncryptedMessage*)message_load(data, size);
	//		printf("Plaintext: %d bytes. Ciphertext: %d bytes\n", msg->plaintext.size, msg->ciphertext.size);
			message_decrypt(msg, &this->server_write_key, &this->server_write_MAC_key, 2, TLS_APPLICATION_DATA);
			
			msg->plaintext.value[msg->plaintext_size+13] = 0;
	/*		for (int i=0; i<msg->plaintext_size; i++) {
				if (msg->plaintext.value[i] == 0x0A) msg->plaintext.value[i] = '[';
			}*/
			TLS_debug = 1;
			printf_win(win, "%s\n", message_plaintext(msg));
			TLS_debug = 0;
			message_free(msg);
			kfree(data);
			keep_downloading = 0;
		};
		TCP_cleanup_connection();
	}

	~TLS() {
	}
};

extern "C" void TLS_init(Window *win, uint ip, char *hostname, uint8 payload[]) {
	TLS(win, ip, hostname, payload);
}
