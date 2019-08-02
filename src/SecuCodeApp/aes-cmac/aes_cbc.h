#ifndef _AES_CBC_
#define _AES_CBC_

#include <stdint.h>

#define B_LAST_INDEX 	15
#define BLOCK_SIZE 		16

int existPadding(unsigned char *block);

void aes_cbc_encript(
	const uint8_t *data, 
	int len, 
	const uint8_t *key,
	const uint8_t *iv,
	const uint8_t *kiv,
	const uint8_t *k1,
	const uint8_t *k2,
	uint8_t *mac
);

void aes_cbc_decript(unsigned char *data, int len, unsigned char *key,unsigned char *iv,unsigned char *k1);

#endif
