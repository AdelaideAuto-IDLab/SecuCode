/**
 * Driver for Physical-DataLink layers using the WISP5 software RFID implementation.
 */

#include <msp430.h>

#include "TI_MSPBoot_Common.h"
#include "TI_MSPBoot_CI.h"
#include "TI_MSPBoot_AppMgr.h"

#include "wisp-base.h"

// Indicates that the tag is attempting to send a byte to the reader.
#define STATE_RX_PENDING (0xAA)

WISP_dataStructInterface_t wispData;
static uint16_t* BASE_ADDRESS = (uint16_t*)0x10000;

extern t_CI_Callback* CI_Callback_ptr;
extern uint8_t CommStatus;

void TI_MSPBoot_CI_PHYDL_Init(t_CI_Callback* CI_Callback)
{
    // Get access to EPC, READ, and WRITE data buffers
    WISP_getDataBuffers(&wispData);

    wispData.epcBuf[0] = 0x0B;        // Tag type
    wispData.epcBuf[1] = 0x7F;        // Tag state field
    wispData.epcBuf[2] = 0;           // Tag tx data field
    wispData.epcBuf[3] = 0;           // Unused data field
    wispData.epcBuf[4] = 0;           // Unused data field
    wispData.epcBuf[5] = 0;           // Unused data field
    wispData.epcBuf[6] = 0;           // Unused data field
    wispData.epcBuf[7] = 0x00;        // Unused data field
    wispData.epcBuf[8] = 0x00;        // Unused data field
    wispData.epcBuf[9] = 0x51;        // Tag hardware revision (5.1)
    wispData.epcBuf[10] = *((uint8_t*)INFO_WISP_TAGID+1); // WISP ID MSB: Pull from INFO seg
    wispData.epcBuf[11] = *((uint8_t*)INFO_WISP_TAGID); // WISP ID LSB: Pull from INFO seg

    CI_Callback_ptr = CI_Callback;
}

void TI_MSPBoot_CI_PHYDL_Poll(void)
{
    static uint8_t* bytes = 0x00;
    static uint8_t* end = 0x00;

    if (bytes == end) {
        // We use EPC[1] as a tag state flag, 0x7F signals to the host that the tag is in
        // "bootloader" mode, and is ready to process new commands
        wispData.epcBuf[1] = 0x7F;

		// Configure the tag to wait for a write to occur
        WISP_setMode(MODE_READ | MODE_WRITE | MODE_USES_SEL);
        WISP_setAbortConditions(0);

        while(wispData.epcBuf[1] != STATE_DONE) {
            WISP_doRFID();
        }

        bytes = (uint8_t*)(&BASE_ADDRESS[1]);
        end = &bytes[BASE_ADDRESS[0]];
    }

    // TODO: Check WISP error flags?


    if (CI_Callback_ptr->RxCallback != NULL) {
        // Call RX Callback (if valid) and sending the byte to upper layer

        while (bytes != end && !(CommStatus & COMM_PACKET_RX)) {
            CI_Callback_ptr->RxCallback(*bytes);
            ++bytes;
        }
    }
}

/**
 * Sends a byte to the reader by setting a particular value in the EPC field
 */
void TI_MSPBoot_CI_PHYDL_TXByte(uint8_t byte) {
    wispData.epcBuf[2] = byte;
    wispData.epcBuf[1] = STATE_RX_PENDING;
}

