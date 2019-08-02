
    .ref TIMER3_A0_ISR, RX_ISR, Timer1A0_ISR, Timer0A1_ISR, Timer0A0_ISR, INT_ADC12
    .if $defined(DEBUG_FIRMWARE)
   		; Debug firmware overrides TIMER3_A0_ISR
    .else
    	.intvec ".int35", TIMER3_A0_ISR
    .endif
    .intvec ".int36", RX_ISR
    .intvec ".int41", Timer1A0_ISR
    .intvec ".int44", Timer0A1_ISR
    .intvec ".int45", Timer0A0_ISR
    .intvec ".int46", INT_ADC12
