#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	__declspec(dllexport) void __cdecl XTEAenc(uint32_t* v, uint32_t* k, int16_t num_rounds) {
		uint32_t y = v[0], z = v[1], DELTA = 0x9e3779b9;

		if (num_rounds > 0) {
			uint32_t limit = DELTA * num_rounds, sum = 0;

			uint16_t i;
			for (i = 0; i < num_rounds; i++) {
				y += ((z << 4 ^ z >> 5) + z) ^ (sum + k[sum & 3]);
				sum += DELTA;
				z += ((y << 4 ^ y >> 5) + y) ^ (sum + k[(sum >> 11) & 3]);
			}
		}

		v[0] = y, v[1] = z;
	}

	__declspec(dllexport) void __cdecl XTEAdec(uint32_t* v, uint32_t* k, int16_t num_rounds) {
		uint32_t y = v[0], z = v[1];
		uint32_t DELTA = 0x9e3779b9;

		if (num_rounds > 0) {
			uint32_t sum = DELTA * num_rounds;

			uint16_t i;
			for (i = 0; i < num_rounds; i++) {
				z -= ((y << 4 ^ y >> 5) + y) ^ (sum + k[(sum >> 11) & 3]);
				sum -= DELTA;
				y -= ((z << 4 ^ z >> 5) + z) ^ (sum + k[sum & 3]);
			}

		}

		v[0] = y, v[1] = z;
	}

#ifdef __cplusplus
}
#endif