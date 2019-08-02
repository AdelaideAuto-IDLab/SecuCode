/*
Copyright (c) 2016, Moritz Bitsch, 2017, Yang Su

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define ROR(x, r) ((x >> r) | (x << ((sizeof(uint32_t) * 8) - r)))
#define ROL(x, r) ((x << r) | (x >> ((sizeof(uint32_t) * 8) - r)))


#define R(x, y, k) (x = ROR(x, 8), x += y, x ^= k, y = ROL(y, 3), y ^= x)
#define RR(x, y, k) (y ^= x, y = ROR(y, 3), x ^= k, x -= y, x = ROL(x, 8))


void SPECK_CORE(uint32_t pt[2], uint32_t ct[2], uint32_t K[4])
{
	uint32_t i, b = K[0];
	uint32_t a[3];
	ct[0] = pt[0]; ct[1] = pt[1];

	a[0] = K[1];
	a[1] = K[2];
	a[2] = K[3];

	R(ct[1], ct[0], b);
	for (i = 0; i < 24; i += 3) {
		R(a[0], b, i);
		R(ct[1], ct[0], b);
		R(a[1], b, i + 1);
		R(ct[1], ct[0], b);
		R(a[2], b, i + 2);
		R(ct[1], ct[0], b);
	}
	R(a[0], b, 24);// rest term out side multiple of 3
	R(ct[1], ct[0], b);
	R(a[1], b, 25);
	R(ct[1], ct[0], b);
}


__declspec(dllexport) void __cdecl HASH_SPECK128(uint64_t nonce, uint8_t firmware[], uint16_t size, uint32_t state[2]) {
	uint16_t idx = 0;
	state[0] = (nonce & 0xffffffff);
	state[1] = (nonce >> 32);
	uint32_t nextState[2] = { 0,0 };
	uint32_t block[4];
	uint16_t residual = size;
	if (size > 16) {
		for (idx = 0; idx < size - 16; idx += 16) {     //first n blocks
														//uint32_t* input = (firmware+(idx*sizeof(uint8_t)));
			memcpy(block, (firmware + (idx * sizeof(uint8_t))), 16);
			SPECK_CORE(state, nextState, block);
			state[0] = nextState[0];
			state[1] = nextState[1];
		}
		residual = size - idx;//how many bytes left not hashed
	}
	else {
		residual = size;
		idx = 0;
	}
	//last block if firmware is not whole multiple of 128 bit
	memcpy(block, (firmware + (idx * sizeof(uint8_t))), residual);
	memset((uint8_t*)block + residual, 0, 16 - residual);
	SPECK_CORE(state, nextState, block);//fill the missing byte with 0
	state[0] = nextState[0];
	state[1] = nextState[1];
}

__declspec(dllexport) void __cdecl testHash(void) {
	uint8_t data[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
	};
	uint8_t state[8];
	HASH_SPECK128(0x0102030405060708, (uint8_t*)data, 0x40, (uint32_t*)state);

	printf("hash = ");
	for (int i = 0; i < 8; ++i) {
		printf("%02x", (unsigned int)state[i]);
		if (i != 7) {
			printf(" ");
		}
	}
	printf("\n\n");
}
