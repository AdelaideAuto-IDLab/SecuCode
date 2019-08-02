/// Use the BlockWrite callback to support a control message interface.

#include "../wisp-base.h"
#include "authenticate.h"
#include <msp430.h>
#include <string.h>
#include <stdint.h>
#ifndef NO_SECURE
#include "aes_cbc.h"
#include "aes256.h"
//#include "speck.h"
//#include "blake2s.h"
#else
#include "crc64.h"
#endif

extern void TI_MSPBoot_AppMgr_RestartInBootMode(void);
extern RWstruct RWData;
extern RFIDstruct rfid;
extern uint8_t helper_data[16 + 2 * 8];
extern uint8_t key[16];
const uint8_t fixedKey[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2,
					0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
uint8_t useFixedKey = 0;

uint16_t prevDecryptOffset;
uint16_t* receivedWords;
uint16_t secuCommMode;

static uint16_t* BASE_ADDRESS = (uint16_t*) 0x10000;

void initAuthenticateState(void) {
	receivedWords = (uint16_t*) (&dataBuf[4]);
	secuCommMode = 0;
	prevDecryptOffset = 0;
}

void decryptCurrentBlock() {
    if (FLAG_SET(secuCommMode, SECU_COMM_IP_PROTECT)
            && FLAG_SET(secuCommMode, SECU_COMM_DECRYPT_AFTER_EACH_PACKET)
            && prevDecryptOffset < RWData.wordPtr) {
        uint8_t* dataPtr = (uint8_t*) (&BASE_ADDRESS[RWData.wordPtr - 8]);
        AES256_decryptData(AES256_BASE, dataPtr, dataPtr);
        prevDecryptOffset = RWData.wordPtr;
    }
}

#ifndef NO_SECURE


void decryptAll(uint16_t* data, uint16_t num_bytes) {
	unsigned int i;
	if (FLAG_SET(secuCommMode, SECU_COMM_IP_PROTECT)) {
		if (FLAG_SET(secuCommMode, SECU_COMM_DECRYPT_AFTER_EACH_PACKET)) {
			// Only the last block needs to be decrypted
			uint8_t* dataPtr = (uint8_t*) (&data[(num_bytes >> 1) - 8]);
			AES256_decryptData(AES256_BASE, dataPtr, dataPtr);
		} else {
			for (i = 0; i < (num_bytes >> 1); i += 8) {
				AES256_decryptData(AES256_BASE, (uint8_t*) &data[i],
						(uint8_t*) &data[i]);
			}
		}
	} else {
		// Only decrypt the HASH, the rest is sent as plain text.
		i = (num_bytes >> 1) - 8;
		AES256_decryptData(AES256_BASE, (uint8_t*) &data[i],
				(uint8_t*) &data[i]);
	}
}

/*
 * validate_HWAES_GMAC128()
 * Para0: data pointer unsigned char*
 * Para1: data size unsigned int16
 * Para2: nonce pointer unsigned char*
 * Para3: nonce size unsigned char
 * Return: hash value matched Yes=1 No=0 boolean
 *
 */
int validate_HWAES_CMAC128(uint8_t *data, uint16_t size, uint8_t *nonce,
		uint8_t nonce_size) {
	uint8_t mac[16];
	uint8_t target_hash[16];
	memcpy(target_hash,&data[size - 16],16);

	if(0 == useFixedKey){
		aes_cbc_encript(data, (size-16), key, nonce, key,key,key, mac);
	}else{
		aes_cbc_encript(data, (size-16), fixedKey, nonce, fixedKey,fixedKey,fixedKey, mac);
	}
	return 0 == memcmp(target_hash, mac, 16); // if calculated hash matches received hash?
}


int validateSpeck(uint16_t* data, uint16_t size) {
	uint64_t nonce = *(uint64_t*) (&helper_data[16]);
	uint32_t state[2];
	//HASH_SPECK128(nonce, (uint8_t *) data, size - 8, state);

	uint16_t hash[4];
	memcpy(hash, state, 8);

	uint16_t* target_hash = &data[(size >> 1) - 4];

	return hash[0] == target_hash[0] && hash[1] == target_hash[1]
			&& hash[2] == target_hash[2] && hash[3] == target_hash[3];
}

int validateBlake(uint16_t* target_hash, uint16_t* computed_hash, size_t length) {
	while (length > 0) {
		if (*target_hash != *computed_hash) {
			return FALSE;
		}

		++target_hash;
		++computed_hash;
		--length;
	}
	return true;
}

int validateBlake128(uint16_t* data, uint16_t size) {
	uint8_t hash[16];
	//blake2s(hash, 16, NULL, 0, data, size - 16);

	uint16_t* target_hash = &data[(size >> 1) - (16 >> 1)];
	uint16_t* computed_hash = (uint16_t*) (hash);

	return validateBlake(target_hash, computed_hash, 8);
}

int validateBlake256(uint16_t* data, uint16_t size) {
	uint8_t hash[32];
	//blake2s(hash, 32, NULL, 0, data, size - 32);

	uint16_t* target_hash = &data[(size >> 1) - (32 >> 1)];
	uint16_t* computed_hash = (uint16_t*) (hash);

	return validateBlake(target_hash, computed_hash, 16);
}

int runHash(uint16_t* data, uint16_t size, uint16_t sleepTime) {
	uint8_t hash[32];
	//blake2s_withSleep(hash, 32, NULL, 0, data, size, sleepTime);
	return 1;
}

#else
int validate_crc64(uint8_t *data, uint16_t size) {
	uint8_t target_crc[8];
	uint8_t calculated_crc[8];
	uint64_t calculated_crc64;
	memcpy(target_crc,&data[size - 8],8);
	calculated_crc64 = crc64(0, data,size);
	uint8_t *p = (uint8_t *)&calculated_crc64;
	uint16_t i;
	for(i = 0; i < 8; i++) {
		calculated_crc[i] = p[i];
	}

	return 0 == memcmp(target_crc, calculated_crc, 8); // if calculated hash matches received hash?
}
#endif

int validateData(uint16_t* data, uint16_t size) {
#ifndef NO_SECURE
	if (FLAG_SET(secuCommMode, SECU_COMM_BLAKE_HASH)) {
		if (FLAG_SET(secuCommMode, SECU_COMM_HASH_LEN_256)) {
			return validateBlake256(data, size);
		}

		return validateBlake128(data, size);
	}
	if (FLAG_SET(secuCommMode, SECU_COMM_GMAC_AES_128)) {
		return validate_HWAES_CMAC128((uint8_t*)data, size, &helper_data[16],
				(NONCE_ZS * 2)); // NONCE_ZS is in words 2*bytes
	}
#else
	if (FLAG_SET(secuCommMode, SECU_COMM_NO_KEY_CRC64)) {
		return validate_crc64((uint8_t*)data,size);
	}
#endif

	return 0;
}

void blockWriteControlMessage(void) {
	if (TAG_STATE == (uint8_t) RWData.wordPtr) {
		// Already received and processed this control message
		return;
	}

	switch ((uint8_t) RWData.wordPtr) {
	case STATE_SET_OFFSET:
		RWData.bwrBufPtr = &BASE_ADDRESS[(RWData.controlMessage & 0x1FFF)];
		*receivedWords = 0;
		prevDecryptOffset = 0;
		break;
#ifndef NO_SECURE
	case STATE_PERFORM_HASH:
		runHash(BASE_ADDRESS, RWData.controlMessage, 200);
		break;
#endif

	case STATE_DONE:
		// Check that we have received the initial authenticate command
		if (!FLAG_SET(secuCommMode, SECU_COMM_RX_AUTHENTICATE)) {
			return;
		}

		// Calculate the correct number of received bytes accounting for offset control messages
		// The address stored in bwrBufPtr is 20-bit, we want the offset from 0x10000, since the
		// highest address allowed is 0x13FFF, only the lower 16-bits matter.
		RWData.bwrByteCount = (uint16_t) RWData.bwrBufPtr
				+ (*receivedWords << 1);

		//decryptAll(BASE_ADDRESS, RWData.bwrByteCount);

		if (!validateData(BASE_ADDRESS, RWData.bwrByteCount)) {
			RWData.bwrByteCount = 0;
			BASE_ADDRESS[0] = 0;
			dataBuf[5] = 0xEE;
		}

		// Exit the main RFID loop to allow the AppManager to process the bytes
		secuCommMode = 0;
		rfid.abortFlag = 1;
		break;

	case STATE_PREPARE_HELPER_DATA:
		if (TAG_STATE != STATE_RESTART_IN_BOOT_MODE) {
			// Not in bootloader mode
			return;
		}

		// TODO: it is trivial for an attacker to perform a buffer overflow attack here and extract
		// the full key from memory.... Need some logic inside of rfid_ReadHandle to validate the
		// bounds of the read request, or to copy the helper data to a safe location.
		RWData.USRBankPtr = &helper_data[0];
		secuCommMode = RWData.controlMessage | SECU_COMM_RX_AUTHENTICATE;
		dataBuf[5] = 0x00;

		if (FLAG_SET(secuCommMode, SECU_COMM_FIXED_KEY)) {
			useFixedKey = 1;
			//AES256_setDecipherKey(AES256_BASE, fixedKey,AES256_KEYLENGTH_128BIT);
		} else {
			useFixedKey = 0;
			//AES256_setDecipherKey(AES256_BASE, key, AES256_KEYLENGTH_128BIT);
		}
		break;

    case STATE_RESTART_IN_BOOT_MODE:
        TI_MSPBoot_AppMgr_RestartInBootMode();
		break;

	default:
		break;
	}

	TAG_STATE = (uint8_t) RWData.wordPtr;
	RWData.wordPtr = 0;
}

void blockWriteRxWord(void) {
	if (RWData.wordPtr == *receivedWords) {
		*receivedWords = RWData.wordPtr + 1;
	}
	TAG_STATE = 0x00;
}

