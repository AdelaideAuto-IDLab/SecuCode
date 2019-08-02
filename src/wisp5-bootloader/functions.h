#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_
#include "wisp-base.h"

// 1 unit = 0.1 mS
#define SLEEPTIME 300

inline void sleeplpm3();
inline void sleeplpm3(uint16_t);

void encode_bch(unsigned char *ri, unsigned char *helper,unsigned char *key);
void SRAMTRNG(uint16_t *nonce,uint8_t nonce_size_byte);


#endif /* FUNCTIONS_H_ */
