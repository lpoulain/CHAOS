#ifndef __CRYPTO_H
#define __CRYPTO_H

#include "libc.h"

int aes_encrypt_cbc(const uint8 in[], uint len, uint8 out[], const uint8 key[], int keysize, const uint8 iv[]);
int aes_decrypt_cbc(const uint8 in[], uint in_len, uint8 out[], const uint8 key[], int keysize, const uint8 iv[]);
void SHA1(const uint8 data[], uint len, uint8 hash[]);
void SHA256(const uint8 data[], uint len, uint8 hash[]);
void HMAC_SHA1(const uint8 text[], int text_len, const uint8 key[], int key_len, uint8 out[]);
void HMAC_SHA256(const uint8 text[], int text_len, const uint8 key[], int key_len, uint8 out[]);

#endif
