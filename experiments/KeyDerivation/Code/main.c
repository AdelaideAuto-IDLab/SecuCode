/**
 * @file       usr.c
 * @brief      WISP application-specific code set
 * @details    The WISP application developer's implementation goes here.
 *
 * @author     Aaron Parks, UW Sensor Systems Lab
 *
 */

#include "wisp-base.h"
#include "functions.h"
#include "puf.h"
#include <msp430.h>

WISP_dataStructInterface_t wispData;
uint8_t helper_data[16];
uint8_t key[16];
uint16_t nonce[4];
//1 number = 0.1 mS
#define SLEEPTIME 300


/**
 * This function is called by WISP FW after a successful ACK reply
 *
 */
void my_ackCallback (void) {
  asm(" NOP");
}

/**
 * This function is called by WISP FW after a successful read command
 *  reception
 *
 */
void my_readCallback (void) {
  asm(" NOP");
}

/**
 * This function is called by WISP FW after a successful write command
 *  reception
 *
 */
void my_writeCallback (void) {
  asm(" NOP");
}

/**
 * This function is called by WISP FW after a successful BlockWrite
 *  command decode

 */
void my_blockWriteCallback  (void) {
  asm(" NOP");
}


void main(void) {

    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;       // Lock LPM5.

    CSCTL0_H = 0xA5;
    CSCTL1 = DCOFSEL_0; //1MHz
    CSCTL2 = SELA__VLOCLK + SELS_3 + SELM_3;
    CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;

    uint8_t ci = 3;
    uint8_t ri[32];
    uint8_t hi[16];         //120bit hi even byte discard the last 1 bit
    uint8_t key2[16];            //128bit key
    uint16_t nonce[4];
    SRAMTRNG(nonce);

    getPUF(ri,ci);              //request response ri from ci -th M-block

    encode_bch(&ri[0],&helper_data[0],&key[0]);
    sleeplpm3();
    encode_bch(&ri[4],&helper_data[2],&key[2]);
    sleeplpm3();
    encode_bch(&ri[8],&helper_data[4],&key[4]);
    sleeplpm3();
    encode_bch(&ri[12],&helper_data[6],&key[6]);
    sleeplpm3();
    encode_bch(&ri[16],&helper_data[8],&key[8]);
    sleeplpm3();
    encode_bch(&ri[20],&helper_data[10],&key[10]);
    sleeplpm3();
    encode_bch(&ri[24],&helper_data[12],&key[12]);
    sleeplpm3();
    encode_bch(&ri[28],&helper_data[14],&key[14]);
    //=-=-=-=At this point=-=-=-=
    //now you have ci[1],hi[16],key[16],nonce[8]
    //note hi is 8x15 bits, discard the last bit in each second byte.
    //key is 8x16 bit

  WISP_init();

  // Register callback functions with WISP comm routines
  WISP_registerCallback_ACK(&my_ackCallback);
  WISP_registerCallback_READ(&my_readCallback);
  WISP_registerCallback_WRITE(&my_writeCallback);
  WISP_registerCallback_BLOCKWRITE(&my_blockWriteCallback);

  // Initialize BlockWrite data buffer.
  uint16_t bwr_array[6] = {0};
  RWData.bwrBufPtr = bwr_array;

  // Get access to EPC, READ, and WRITE data buffers
  WISP_getDataBuffers(&wispData);

  // Set up operating parameters for WISP comm routines
  WISP_setMode( MODE_READ | MODE_WRITE | MODE_USES_SEL);
  WISP_setAbortConditions(CMD_ID_READ | CMD_ID_WRITE | CMD_ID_ACK);

  // Set up EPC
  wispData.epcBuf[0] = 0x0B;        // Tag type
  wispData.epcBuf[1] = 0;           // Unused data field
  wispData.epcBuf[2] = ci&0xFF;
  wispData.epcBuf[3] = (ci>>8);
  wispData.epcBuf[4] = 0;           // Unused data field
  wispData.epcBuf[5] = 0;           // Unused data field
  wispData.epcBuf[6] = 0;           // Unused data field
  wispData.epcBuf[7] = 0x00;        // Unused data field
  wispData.epcBuf[8] = 0x00;        // Unused data field
  wispData.epcBuf[9] = 0x51;        // Tag hardware revision (5.1)
  wispData.epcBuf[10] = *((uint8_t*)INFO_WISP_TAGID+1); // WISP ID MSB: Pull from INFO seg
  wispData.epcBuf[11] = *((uint8_t*)INFO_WISP_TAGID); // WISP ID LSB: Pull from INFO seg

  // Talk to the RFID reader.
  while (FOREVER) {
    WISP_doRFID();
    while(1){}
  }
}


// Some symbols that need to be defined for the bootloader to work
void TI_MSPBoot_AppMgr_RestartInBootMode(void) {}
