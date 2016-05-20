#include "libc.h"
#include "crypto.h"

#define SHA256_DIGEST_LENGTH 32
#define SHA1_DIGEST_LENGTH 20

typedef void (*crypto_hash)(uint8 data[], uint len, uint8 hash[]);

void HMAC(const uint8 text[], int text_len, const  uint8 key[], int key_len, uint8 out[], crypto_hash hash, uint digest_length)
{
    unsigned char k_ipad[65];   /* inner padding -
                                 * key XORd with ipad
                                 */
    unsigned char k_opad[65];   /* outer padding -
                                 * key XORd with opad
                                 */
    unsigned char tk[digest_length];
    unsigned char tk2[digest_length];
    unsigned char bufferIn[1024];
    unsigned char bufferOut[1024];
    int           i;

    /* if key is longer than 64 bytes reset it to key=sha256(key) */
    if ( key_len > 64 ) {
        hash( key, key_len, tk );
        key     = tk;
        key_len = digest_length;
    }

    /*
     * the HMAC_SHA256 transform looks like:
     *
     * SHA256(K XOR opad, SHA256(K XOR ipad, text))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated 64 times
     * opad is the byte 0x5c repeated 64 times
     * and text is the data being protected
     */

    /* start out by storing key in pads */
    memset( k_ipad, 0, sizeof k_ipad );
    memset( k_opad, 0, sizeof k_opad );
    memcpy( k_ipad, key, key_len );
    memcpy( k_opad, key, key_len );

    /* XOR key with ipad and opad values */
    for ( i = 0; i < 64; i++ ) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /*
     * perform inner SHA256
     */
    memset( bufferIn, 0x00, 1024 );
    memcpy( bufferIn, k_ipad, 64 );
    memcpy( bufferIn + 64, text, text_len );

    hash( bufferIn, 64 + text_len, tk2 );

    /*
     * perform outer SHA256
     */
    memset( bufferOut, 0x00, 1024 );
    memcpy( bufferOut, k_opad, 64 );
    memcpy( bufferOut + 64, tk2, digest_length );

    hash( bufferOut, 64 + digest_length, out );
}

void HMAC_SHA256(const uint8 text[], int text_len, const  uint8 key[], int key_len, uint8 out[]) {
	HMAC(text, text_len, key, key_len, out, SHA256, SHA256_DIGEST_LENGTH);
}

void HMAC_SHA1(const uint8 text[], int text_len, const  uint8 key[], int key_len, uint8 out[]) {
	HMAC(text, text_len, key, key_len, out, SHA1, SHA1_DIGEST_LENGTH);
}
