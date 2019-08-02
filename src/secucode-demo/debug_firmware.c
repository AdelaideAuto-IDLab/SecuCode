#include "ota_demo.h"

#ifdef DEBUG_FIRMWARE

#define OFF_DURATION (40000)
#define ON_DURATION (2000)

#pragma NOINIT (next_duration)
uint16_t next_duration;

extern WISP_dataStructInterface_t wispData;

inline void epcInit(void) {
    WISP_getDataBuffers(&wispData);

    wispData.epcBuf[0] = 0x0B;        // Tag type
    wispData.epcBuf[1] = 0xFF;        // Tag state field
    wispData.epcBuf[9] = 0x51;        // Tag hardware revision (5.1)
    wispData.epcBuf[10] = *((uint8_t*)INFO_WISP_TAGID+1); // WISP ID MSB: Pull from INFO seg
    wispData.epcBuf[11] = *((uint8_t*)INFO_WISP_TAGID); // WISP ID LSB: Pull from INFO seg
}

int main(void) {
    // Stop watchdog timer to prevent time out reset
    WDTCTL = WDTPW + WDTHOLD;

    epcInit();

    // Set timer interrupt
    // The WISP 5 firmware is not really designed to handle random external interrupts however
    // in practice we should be fine as long as the interrupts do not occur too often.
    next_duration = OFF_DURATION;
    TA3CCTL0 = CCIE;
    TA3CCR0 = next_duration;
    TA3CTL = TASSEL_1 + MC_1;

    while (1) {
        WISP_setMode(MODE_READ | MODE_WRITE | MODE_USES_SEL);
        WISP_setAbortConditions(0);
        WISP_doRFID();
    }
}

#pragma vector = TIMER3_A0_VECTOR
__interrupt void Timer_3 (void) {
    PLED1OUT ^= PIN_LED1;
    next_duration = (ON_DURATION + OFF_DURATION) - next_duration;
    TA3CCR0 = next_duration;
    TA3CCTL0 &= ~CCIFG;
}
#endif
