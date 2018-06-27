#include "wisp-base.h"
#include <msp430.h>

extern char BYTES_TO_HASH[1280];
int runHash(uint16_t* data, uint16_t size, uint16_t sleepTime);

WISP_dataStructInterface_t wispData;
uint8_t helper_data[16];
uint8_t key[16];

//1 number = 0.1 mS
#define SLEEPTIME 300
#define HASH_LENGTH 1280

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;       // Lock LPM5.

    CSCTL0_H = 0xA5;
    CSCTL1 = DCOFSEL_0; //1MHz
    CSCTL2 = SELA__VLOCLK + SELS_3 + SELM_3;
    CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;

    const uint16_t size = 1280;
    runHash(BYTES_TO_HASH, HASH_LENGTH, SLEEPTIME);

    WISP_init();

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
    wispData.epcBuf[2] = 0;
    wispData.epcBuf[3] = 0;
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
#pragma vector=TIMER3_A0_VECTOR
__interrupt void TIMER3_A0_ISR(void)
{
    __bic_SR_register_on_exit(LPM3_bits|GIE);
}
