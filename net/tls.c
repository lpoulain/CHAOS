#include "libc.h"
#include "display.h"
#include "tcp.h"
#include "kheap.h"
#include "crypto.h"

#define TLS_VERSION							0x0303
#define TLS_HANDSHAKE						0x16
#define TLS_CHANGE_CIPHER_SPEC				0x14
#define TLS_APPLICATION_DATA				0x17
#define TLS_HANDSHAKE_CLIENT_HELLO			0x01
#define TLS_HANDSHAKE_SERVER_HELLO			0x02
#define TLS_HANDSHAKE_CERTIFICATE			0x0B
#define TLS_HANDSHAKE_SERVER_KEY_EXCHANGE	0x0C
#define TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE	0x10
#define TLS_HANDSHAKE_SERVER_HELLO_DONE		0x0E

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

typedef struct __attribute__((packed)) {
	uint16 size;
	uint8 *value;
} TLSNumber;

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

void TLSNumber_print(TLSNumber *nb) {
	for (int i=0; i<nb->size; i++)
		printf("%X", nb->value[i]);
	printf("\n");
}

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


// Returns a 32-bytes result
void HMAC_hash(TLSNumber *key, TLSNumber *secret, uint8 out[]) {
	HMAC_SHA256(secret->value, secret->size, key->value, key->size, out);
}

// Returns a result whose size is a multiple of 32 bytes
void P_hash(TLSNumber *secret, TLSNumber *seed, uint8 out[], uint output_size) {
	// A (initially 'seed', but after the first iteration will always be 32 bytes)
	TLSNumber A;
	A.value = seed->value;
	A.size = seed->size;
	uint8 A_data[32];

	// A + seed (always 32+seed's size bytes)
	TLSNumber A_plus_seed;
	uint8 *A_plus_seed_value = kmalloc(seed->size + 32);
	A_plus_seed.value = A_plus_seed_value;
	A_plus_seed.size = seed->size + 32;

	uint8 hash[32];
	uint result_offset = 0;

	while (output_size > 0) {
		// A = HMAD_hash(secret, A)
		HMAC_hash(secret, &A, hash);
		A.value = (uint8*)&A_data;
		A.size = 32;
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

void PRF(TLSNumber *secret, char *label, TLSNumber *seed1, TLSNumber *seed2, uint8 out[], uint output_size) {
	TLSNumber new_seed;
	uint label_size = strlen(label);
	new_seed.size = label_size + seed1->size;
	if (seed2 != 0) new_seed.size += seed2->size;

	new_seed.value = (uint8*)kmalloc(new_seed.size);
	memcpy(new_seed.value, label, label_size);
	memcpy(new_seed.value + label_size, seed1->value, seed1->size);
	if (seed2 != 0) memcpy(new_seed.value + label_size + seed1->size, seed2->value, seed2->size);

	P_hash(secret, &new_seed, out, output_size);

	kfree(new_seed.value);
}

/////////////////////////////////////////////////////

typedef struct {
	TLSNumber plaintext;	// Contains the plaintext + other data
	uint16 plaintext_size;	// The official plaintext size
	TLSNumber ciphertext;	// The ciphertext as sent through TCP/IP
} TLSEncryptedMessage;

TLSEncryptedMessage *message_new(uint8 content_type, uint16 size) {
	TLSEncryptedMessage *msg = (TLSEncryptedMessage*)kmalloc(sizeof(TLSEncryptedMessage));

	msg->plaintext.size = 13 + 16 * ((size + 20) / 16) + 16;				// 13 bytes are used to compute the MAC
																	// size + 20: plaintext + MAC
																	// (size + 20) / 16 + 16: plaintext + MAC + CBC padding
	msg->plaintext.value = kmalloc(msg->plaintext.size);	
	msg->plaintext_size = size;

	msg->ciphertext.size = 16 * ((size + 20) / 16) + 32 + 5;					// IV + plaintext + MAC + CBC padding
	msg->ciphertext.value = kmalloc(5 + msg->ciphertext.size);					// 5 bytes for the TLS header
	msg->ciphertext.value[0] = content_type;
	msg->ciphertext.value[1] = 0x03;
	msg->ciphertext.value[2] = 0x03;
	msg->ciphertext.value[3] = (uint8)((msg->ciphertext.size - 5) >> 8) & 0xFF;
	msg->ciphertext.value[4] = (uint8)((msg->ciphertext.size - 5) & 0xFF);

	return msg;
}

uint8 *message_load(uint8 *ciphertext, uint16 size) {
	TLSEncryptedMessage *msg = (TLSEncryptedMessage*)kmalloc(sizeof(TLSEncryptedMessage));

	msg->ciphertext.value = ciphertext;
	msg->ciphertext.size = size;

	msg->plaintext.size = size - 16 + 13;		// The plaintext doesn't contain the IV (16 bytes)
												// But for compatibility we need the 13-bytes overhead
	msg->plaintext.value = kmalloc(msg->plaintext.size);
	msg->plaintext_size = size - 16;

	return msg;
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
	kfree(msg->plaintext.value);
	kfree(msg->ciphertext.value);
	kfree(msg);
}

// Ciphertext: IV | AES(16-bytes plaintext | 20-bytes HMAC+SHA1 | 12-bytes padding)
void message_encrypt(TLSEncryptedMessage *msg, uint8 iv[], TLSNumber *AES_key, TLSNumber *MAC_key, uint seq_num, uint8 content_type) {
	// Writes the first 13 bytes, which are used to compute the MAC
	uint *tmp32 = msg->plaintext.value;
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

///////////////////////////////////////////////////////////////////////////////////////////////

void TLS_init(Window *win, uint ip, char *hostname, uint8 payload[]) {

	///////////////////////////////
	// Sends Client Hello message
	///////////////////////////////

	uint8 client_hello[50] = {
		0, 0, 0, 0, 0,		// TLS Record header
		0, 0, 0, 0, 0, 0,	// TLS Handshake header
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// random data
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// random data
		0,				// session ID length
		0x00, 0x02,		// number of cipher suites supported
		0x00, 0x33,		// supported cipher suite: TLS_DHE_RSA_WITH_AES_128_CBC_SHA
		0x01, 0x00 		// no compression
	};
	
	TLSNumber client_random;
	client_random.value = (uint8*)&client_hello + 11;
	client_random.size = 32;

	for (int i=0; i<32; i++) {
		client_random.value[i] = rand() % 256;
	}

	TLSRecord *record = (TLSRecord*)&client_hello;
	record->content_type = TLS_HANDSHAKE;
	record->version = TLS_VERSION;
	record->length = switch_endian16(45);

	TLSHandshake *handshake = (TLSHandshake*)((uint8*)&client_hello + sizeof(TLSRecord));
	handshake->handshake_type = TLS_HANDSHAKE_CLIENT_HELLO;
	handshake->length = switch_endian16(41);
	handshake->version = TLS_VERSION;

	// We will need to copy the complete handshake in a single allocation
	// to compute the MAC. We thus need to compute its size
	// We first add the Client Hello message size
	uint handshake_size = 45;

	TCPConnection *connection = TCP_start_connection(ip, TCP_PORT_HTTPS, (uint8*)&client_hello, 50);

	////////////////////////////////////////////////////////////////////////
	// Receives Server Hello, Certificate and Server Key Exchange messages
	////////////////////////////////////////////////////////////////////////

	TLSCursor cursor;
	TLSCursor_init(&cursor, connection);
	uint8 keep_downloading = 1;
	uint16 size;

	while (keep_downloading) {
		TLSRecord *record = (TLSRecord *)TLSCursor_current(&cursor);
		size = switch_endian16(record->length);
//		printf("Content type: %X, size:%d\n", record->content_type, size);
		uint8 *tmp = TLSCursor_next(&cursor, 5);
//		printf("Content subtype: %X\n", tmp[0]);
		handshake_size += size;

		if (tmp[0] == TLS_HANDSHAKE_SERVER_HELLO_DONE) {
			keep_downloading = 0;
//			break;
		}
		else TLSCursor_next(&cursor, size);
	};

	// Allocates the handshake_buffer, which concatenates all the handshake
	// messages
	handshake_size += 64;
	handshake_size += 14;
	handshake_size += 5 + 256;	// Client Key Exchange

	uint8 *handshake_buffer = (uint8*)kmalloc(handshake_size);
//	printf("Allocating %d bytes\n", handshake_size);
//	printf("Handshake: %x->%x\n", handshake_buffer, handshake_buffer+handshake_size);
	memcpy(handshake_buffer, client_hello + 5, 45);
	handshake_size = 45;

	// Second pass: we go through to copy

	uint8 *server_hello, *server_certificate, *server_key_exchange, *server_hello_done;
	TLSCursor_init(&cursor, connection);
	keep_downloading = 1;

	while (keep_downloading) {
		TLSRecord *record = (TLSRecord *)TLSCursor_current(&cursor);
		size = switch_endian16(record->length);
//		printf("Content type: %X, size:%d\n", record->content_type, size);
		uint8 *tmp = TLSCursor_next(&cursor, 5);
//		printf("Content subtype: %X\n", tmp[0]);

		switch(tmp[0]) {
			case TLS_HANDSHAKE_SERVER_HELLO:
				server_hello = handshake_buffer + handshake_size;
				break;
			case TLS_HANDSHAKE_CERTIFICATE:
				server_certificate = handshake_buffer + handshake_size;
				break;
			case TLS_HANDSHAKE_SERVER_KEY_EXCHANGE:
				server_key_exchange = handshake_buffer + handshake_size;
				break;
			case TLS_HANDSHAKE_SERVER_HELLO_DONE:
				server_hello_done = handshake_buffer + handshake_size;
				keep_downloading = 0;
				break;
		}

		TLSCursor_copy_next(&cursor, size, handshake_buffer + handshake_size);
		handshake_size += size;
	};


	// Copy the Server Hello Done message
	memcpy(handshake_buffer + handshake_size - 4, server_hello_done, 4);

//	dump_mem(server_key_exchange, 300, 14);

	TLSNumber server_p, server_g, server_g_y;

	TLSNumber server_random;
	server_random.value = server_hello + 6;
	server_random.size = 32;

	server_p.value = server_key_exchange + 6;
	server_p.size = server_key_exchange[4] * 256 + server_key_exchange[5];
//	printf("[%d]\n", server_p.size);
	server_g.value = server_p.value + 2 + server_p.size;
	server_g.size = *(server_p.value + server_p.size) * 256 + *(server_p.value + server_p.size + 1);
	server_g_y.value = server_g.value + 2 + server_g.size;
	server_g_y.size = *(server_g.value + server_g.size) * 256 + *(server_g.value + server_g.size + 1);

//	printf("p=%X %X %X %X... (%d)\n", server_p.value[0], server_p.value[1], server_p.value[2], server_p.value[3], server_p.size);
//	printf("g=%X (%d)\n", server_g.value[0], server_g.size);
//	printf("g^y=%X %X (%d)\n", server_g_y.value[0], server_g_y.value[1], server_g_y.size);

	////////////////////////////////////////////////////////////////////////
	// Calculates the private keys
	////////////////////////////////////////////////////////////////////////


	// Hard-coding some values for testing
/*	const char g_y_hex[] = "3ce3569a5a65412a69eafb4a14d6282fb9f4a3dc600ec48529dc6a53cc6b6c3ad177a3255d3643e533afad9d7b0ad561b0379fd2f1f105868bc108edbd82a893863eb4303c6bb2f47a501448867431e4edc0d049192e10e0ab370fc4dcea3132475e5eb4ea32f79aae7c949c0f646ad8d11fc71dcdd0c86d89e3ecd980e7b0f2da5df3d3165c7d996005b17afc1711c638e0bb0243f6e4c51fdfaa88e97b79f3354f1b16d0c66b5f9a32315c4d4025da2fc8db838c739e07c31c500435f18d28140ae521dbed1109bee433ab2fff8935344162f6c36c0ef6b7d06f8cfeadd24fea0f1afc1017f5275761c2a7f628130e5473cbccfc7267cf2c8a218b8f036c96";
	const char client_random_hex[] = "bb9e93a92a281773316414acf2c6c6d4cf3fdeeacc6a4373a17c8d4aa2e83e2b";
	const char server_random_hex[] = "572802a94b519314c57b61c389e55298fd4fd743b21012d9b072f5170b523279";

	for (int i=0; i<32; i++) {
		client_random.value[i] = atoi_hex_c(client_random_hex + i*2);
		server_random.value[i] = atoi_hex_c(server_random_hex + i*2);
	}

	for (int i=0; i<256; i++) {
		server_g_y.value[i] = atoi_hex_c(g_y_hex + i*2);
	}
*/
	TLSNumber *premaster_secret = &server_g_y;
	TLSNumber master_secret;
	uint8 master_secret_value[64];
	master_secret.value = master_secret_value;
	master_secret.size = 48;

	PRF(premaster_secret, "master secret", &client_random, &server_random, master_secret.value, 64);

	uint8 keys[128];

	PRF(&master_secret, "key expansion", &server_random, &client_random, keys, 128);
//	printf("Master secret:\n");
//	TLSNumber_print(&master_secret);

	TLSNumber client_write_MAC_key;
	client_write_MAC_key.value = keys;
	client_write_MAC_key.size = 20;

	TLSNumber server_write_MAC_key;
	server_write_MAC_key.value = keys+20;
	server_write_MAC_key.size = 20;

	TLSNumber client_write_key;
	client_write_key.value = keys+40;
	client_write_key.size = 16;

	TLSNumber server_write_key;
	server_write_key.value = keys+56;
	server_write_key.size = 16;

	TLSNumber client_write_IV;
	client_write_IV.value = keys+72;
	client_write_IV.size = 16;

	TLSNumber server_write_IV;
	server_write_IV.value = keys+88;
	server_write_IV.size = 16;
/*
	printf("AES Key:\n");
	TLSNumber_print(&client_write_key);
	printf("MAC Key:\n");
	TLSNumber_print(&client_write_MAC_key);
*/

	////////////////////////////////////////////////////////////////////////
	// Sends Client Key Exchange
	////////////////////////////////////////////////////////////////////////
	uint8 *client_key_exchange = (uint8*)kmalloc(11 + server_p.size);

	// TLS Record header
	record = (TLSRecord*)client_key_exchange;
	record->content_type = TLS_HANDSHAKE;
	record->version = TLS_VERSION;
	record->length = switch_endian16(6 + server_p.size);

	// TLS Handshake header
	handshake = (TLSHandshake*)(client_key_exchange + sizeof(TLSRecord));
	handshake->handshake_type = TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE;
	handshake->padding = 0;
	handshake->length = switch_endian16(server_p.size + 2);
	// Diffie-Hellman parameters
	uint16 *DH_length = (uint16*)(client_key_exchange + 9);
	*DH_length = switch_endian16(server_p.size);
	uint8 *client_g_x = client_key_exchange + 11;
	memset(client_g_x, 0, server_p.size);
	// In order to avoid to compute modular exponentiation, we're using x = 1
	// So the key is g^y
	client_g_x[server_p.size-1] = server_g.value[0];

	memcpy(handshake_buffer + handshake_size, client_key_exchange + 5, 6 + server_p.size);
	handshake_size += 6 + server_p.size;

	TCP_send(client_key_exchange, 11 + server_p.size);

	////////////////////////////////////////////////////////////////////////
	// Sends Change Cipher Spec
	////////////////////////////////////////////////////////////////////////
	uint8 client_change_cipher_spec[6] = { 0x14, 0x03, 0x03, 0x00, 0x01, 0x01 };
	TCP_send(client_change_cipher_spec, 6);

	////////////////////////////////////////////////////////////////////////
	// Sends Encrypted Handshake Message
	////////////////////////////////////////////////////////////////////////
	TLSEncryptedMessage *msg = message_new(TLS_HANDSHAKE, 16);

	uint8* plaintext = message_plaintext(msg);
	plaintext[0] = 0x14;
	plaintext[1] = 0x00;
	plaintext[2] = 0x00;
	plaintext[3] = 0x0C;

	// Computes the hash of the handshake messages
	TLSNumber handshake_msg_hash;
	uint8 handshake_msg_hash_data[32];
	handshake_msg_hash.value = &handshake_msg_hash_data;
	handshake_msg_hash.size = 32;
	SHA256(handshake_buffer, handshake_size, &handshake_msg_hash_data);

	// The PRF will overwrite beyond the 12 bytes we want, but the MAC will be computed afterwards
//	printf("msg=%x, hash=>%x\n", &plaintext_data, (uint8*)(plaintext.value) + 13 + 4);
	PRF(&master_secret, "client finished", &handshake_msg_hash, 0, plaintext + 4, 32);

//	printf("\n%x %x\n\n", &client_encrypted_handshake_message, ciphertext.value);
	message_encrypt(msg, client_write_IV.value, &client_write_key, &client_write_MAC_key, 0, TLS_HANDSHAKE);

	TCP_cleanup_connection();
	TLSCursor_init(&cursor, connection);

	TCP_send(message_all(msg), msg->ciphertext.size);
	message_free(msg);

	////////////////////////////////////////////////////////////////////////
	// Receives Change Cipher Spec, Encrypted Handshake Message
	////////////////////////////////////////////////////////////////////////

	keep_downloading = 1;

	while (keep_downloading) {
		TLSRecord *record = (TLSRecord *)TLSCursor_current(&cursor);
		size = switch_endian16(record->length);
//		printf("Content type: %X, size:%d\n", record->content_type, size);
		uint8 *tmp = TLSCursor_next(&cursor, 5);
//		printf("Content subtype: %X\n", tmp[0]);

		if (record->content_type == TLS_CHANGE_CIPHER_SPEC) keep_downloading = 0;

		TLSCursor_next(&cursor, size);
	};

	TCP_cleanup_connection();
	TLSCursor_init(&cursor, connection);

	////////////////////////////////////////////////////////////////////////
	// Sends the GET request (encrypted)
	////////////////////////////////////////////////////////////////////////
	uint16 payload_size = strlen(payload);
//	uint16 hostname_size = strlen(hostname), payload_size = ;
	msg = message_new(TLS_APPLICATION_DATA, payload_size);

	plaintext = message_plaintext(msg);
	memcpy(plaintext, payload, payload_size);
/*	memcpy(plaintext, "GET / HTTP/1.1\r\nHOST: ", 22);
	memcpy(plaintext+22, hostname, hostname_size);
	memcpy(plaintext+22+hostname_size, "\r\n\r\n", 4);*/

	message_encrypt(msg, client_write_IV.value, &client_write_key, &client_write_MAC_key, 1, TLS_APPLICATION_DATA);

	TCP_send(message_all(msg), msg->ciphertext.size);

	message_free(msg);

	////////////////////////////////////////////////////////////////////////
	// Receives the HTML (encrypted)
	////////////////////////////////////////////////////////////////////////
	keep_downloading = 1;

	while (keep_downloading) {
		TLSRecord *record = (TLSRecord *)TLSCursor_current(&cursor);
		size = switch_endian16(record->length);
//		printf("Content type: %X, size:%d\n", record->content_type, size);
		uint8 *tmp = TLSCursor_next(&cursor, 5);
//		printf("Content subtype: %X\n", tmp[0]);

		if (record->content_type == TLS_CHANGE_CIPHER_SPEC) keep_downloading = 0;

		uint8 *data = kmalloc(size);
		TLSCursor_copy_next(&cursor, size, data);
		msg = message_load(data, size);
//		printf("Plaintext: %d bytes. Ciphertext: %d bytes\n", msg->plaintext.size, msg->ciphertext.size);
		message_decrypt(msg, &server_write_key, &server_write_MAC_key, 2, TLS_APPLICATION_DATA);
		kheap_check_for_corruption();
		msg->plaintext.value[msg->plaintext_size+13] = 0;
/*		for (int i=0; i<msg->plaintext_size; i++) {
			if (msg->plaintext.value[i] == 0x0A) msg->plaintext.value[i] = '[';
		}*/
		TLS_debug = 1;
		printf_win(win, "%s\n", message_plaintext(msg));
		TLS_debug = 0;
//		TLSNumber_print(&msg->plaintext);
		message_free(msg);
		kfree(data);
		keep_downloading = 0;
	};

	TCP_cleanup_connection();
}
