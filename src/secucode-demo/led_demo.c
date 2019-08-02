#include "ota_demo.h"

#ifdef LED_DEMO

#define OFF_DURATION (20000)
#define ON_DURATION (2000)

#pragma NOINIT (next_duration)
uint16_t next_duration;

int main(void) {
    WDTCTL = WDTPW + WDTHOLD;

    // Pre-configured by boot process
    // PM5CTL0 &= ~LOCKLPM5;
    // setupDflt_IO();

    // Set timer
    next_duration = OFF_DURATION;
    TA0CCTL0 = CCIE;
    TA0CCR0 = next_duration;
    TA0CTL = TASSEL_1 + MC_1;

    while (1) {
        __bis_SR_register(LPM3_bits + GIE);
        __no_operation();
    }
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void) {
    PLED1OUT ^= PIN_LED1;
    next_duration = (ON_DURATION + OFF_DURATION) - next_duration;
    TA0CCR0 = next_duration;
    TA0CCTL0 &= ~CCIFG;
}
#endif
