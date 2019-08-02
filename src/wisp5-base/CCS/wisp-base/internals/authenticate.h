#ifndef INTERNALS_AUTHENTICATE_H_
#define INTERNALS_AUTHENTICATE_H_

#define SECU_COMM_IP_PROTECT (1)
#define SECU_COMM_FIXED_KEY (2)
#define SECU_COMM_BLAKE_HASH (4)
#define SECU_COMM_HASH_LEN_256 (8)
#define SECU_COMM_DECRYPT_AFTER_EACH_PACKET (16)
#define SECU_COMM_GMAC_AES_128 (32)
#define SECU_COMM_NO_KEY_CRC64 (64)
#define SECU_COMM_RX_AUTHENTICATE (128)

#define FLAG_SET(x, flag) (((x) & (flag)) == (flag))

#define STATE_SET_OFFSET (1)
#define STATE_DONE (2)
#define STATE_PREPARE_HELPER_DATA (3)  // aka Authenticate
#define STATE_PERFORM_HASH (4)
#define STATE_RESTART_IN_BOOT_MODE (0x7F)

#include <stdint.h>

#define NONCE_ZS   8// nonce size in 16bit words

void initAuthenticateState(void);
void blockWriteControlMessage(void);
void blockWriteRxWord(void);
void decryptCurrentBlock(void);

#define TAG_STATE (dataBuf[3])

#endif /* INTERNALS_AUTHENTICATE_H_ */
