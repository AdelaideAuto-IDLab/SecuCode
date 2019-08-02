#include "ota_demo.h"

#ifdef ACCEL_DEMO

extern WISP_dataStructInterface_t wispData;


inline void fullInitAccel(threeAxis_t_8* accelOut) {
    BITSET(POUT_ACCEL_EN , PIN_ACCEL_EN);

    accelOut->x = 0;
    accelOut->y = 0;
    accelOut->z = 0;

    BITSET(P2SEL1 , PIN_ACCEL_SCLK | PIN_ACCEL_MISO | PIN_ACCEL_MOSI);
    BITCLR(P2SEL0 , PIN_ACCEL_SCLK | PIN_ACCEL_MISO | PIN_ACCEL_MOSI);
    __delay_cycles(5);
    SPI_initialize();
    __delay_cycles(5);
    ACCEL_range();
    __delay_cycles(5);
    ACCEL_initialize();
    __delay_cycles(10);
}

inline void setClock(void) {
    CSCTL0_H = 0xA5;
    CSCTL1 = DCOFSEL_0; //1MHz
    CSCTL2 = SELA__VLOCLK + SELS_3 + SELM_3;
    CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;
    BITCLR(CSCTL6, (MODCLKREQEN|SMCLKREQEN|MCLKREQEN));
    BITSET(CSCTL6, ACLKREQEN);
}

inline void epcInit(void) {
    WISP_getDataBuffers(&wispData);

    wispData.epcBuf[0] = 0x0B;        // Tag type
    wispData.epcBuf[1] = 0xA1;        // Tag state field
    wispData.epcBuf[9] = 0x51;        // Tag hardware revision (5.1)
    wispData.epcBuf[10] = *((uint8_t*)INFO_WISP_TAGID+1); // WISP ID MSB: Pull from INFO seg
    wispData.epcBuf[11] = *((uint8_t*)INFO_WISP_TAGID); // WISP ID LSB: Pull from INFO seg
}

int main(void) {
    // Stop watchdog timer to prevent time out reset
    WDTCTL = WDTPW + WDTHOLD;

    threeAxis_t_8 accelOut;

    fullInitAccel(&accelOut);
    epcInit();

    int i = 0;

    while (1) {
        setClock();

        ACCEL_readStat(&accelOut);
        if((((uint8_t)accelOut.x) & 193) == 0x41){
            __delay_cycles(20);
            ACCEL_singleSample(&accelOut);
            wispData.epcBuf[3] = 0;         // Y value MSB
            wispData.epcBuf[4] = (accelOut.y+128);// Y value LSB
            wispData.epcBuf[5] = 0;         // X value MSB
            wispData.epcBuf[6] = (accelOut.x+128);// X value LSB
            wispData.epcBuf[7] = 0;         // Z value MSB
            wispData.epcBuf[8] = (accelOut.z+128);// Z value LSB
        }

        if (i == 100) {
            // Every so often avoid exiting on a ACK command to allow the tag to process control
            // commands, note that we will still eventually exit due to the RFID timeout.
            WISP_setAbortConditions(CMD_ID_READ | CMD_ID_WRITE | CMD_ID_TIMEOUT);
            i = 0;
        }
        else {
            WISP_setAbortConditions(CMD_ID_READ | CMD_ID_WRITE | CMD_ID_ACK);
            ++i;
        }

        WISP_setMode(MODE_READ | MODE_WRITE | MODE_USES_SEL);
        WISP_doRFID();
    }
}

#endif
