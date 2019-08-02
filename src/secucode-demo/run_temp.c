#include "ota_demo.h"

#ifdef TEMP_DEMO

extern WISP_dataStructInterface_t wispData;

inline void epcInit(void) {
    WISP_getDataBuffers(&wispData);

    wispData.epcBuf[0] = 0x0B;        // Tag type
    wispData.epcBuf[1] = 0xA0;        // Tag state field
    wispData.epcBuf[9] = 0x51;        // Tag hardware revision (5.1)
    wispData.epcBuf[10] = *((uint8_t*)INFO_WISP_TAGID+1); // WISP ID MSB: Pull from INFO seg
    wispData.epcBuf[11] = *((uint8_t*)INFO_WISP_TAGID); // WISP ID LSB: Pull from INFO seg
}

inline void setClock(void) {
    CSCTL0_H = 0xA5;
    CSCTL1 = DCOFSEL_0; //1MHz
    CSCTL2 = SELA__VLOCLK + SELS_3 + SELM_3;
    CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;
    BITCLR(CSCTL6, (MODCLKREQEN|SMCLKREQEN|MCLKREQEN));
    BITSET(CSCTL6, ACLKREQEN);
}

int main(void) {
    // Stop watchdog timer to prevent time out reset
    WDTCTL = WDTPW + WDTHOLD;

    ADC_initCustom(ADC_reference_2_0V, ADC_precision_10bit, ADC_input_temperature);
    epcInit();

    int i = 0;

    while (1) {
        setClock();
        uint16_t adc_value = ADC_read();
        int16_t adc_temperature = ADC_rawToTemperature(adc_value);

        wispData.epcBuf[7] = (adc_temperature >> 8) & 0xFF;
        wispData.epcBuf[8] = (adc_temperature >> 0) & 0xFF;


        if (i == 100) {
            // Every so often avoid exiting on a ACK command to allow the tag to process control
            // commands, note that we will still eventually exit due to the RFID timeout.
            WISP_setAbortConditions(CMD_ID_READ | CMD_ID_WRITE | CMD_ID_TIMEOUT);
            i = 0;
        }
        else {
            // Otherwise exit the RFID loop immediately to sample the next value
            WISP_setAbortConditions(CMD_ID_READ | CMD_ID_WRITE | CMD_ID_ACK);
            ++i;
        }

        WISP_setMode(MODE_READ | MODE_WRITE | MODE_USES_SEL);
        WISP_doRFID();
    }
}

#endif
