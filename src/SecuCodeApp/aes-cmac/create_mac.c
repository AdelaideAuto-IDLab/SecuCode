#include <stdint.h>

#include "aes_cbc.h"

__declspec(dllexport) int __cdecl generate_AES_CMAC(
	const uint8_t* key,
	const uint8_t* nonce,
	const uint8_t* data,
	const uint16_t data_len,
	uint8_t* mac_out)
{
	aes_cbc_encript(data, data_len, key, nonce, key, key, key, mac_out);
	return 0;
}