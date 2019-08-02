/*
 * \file   main.c
 *
 * \brief  Main routine for the bootloader for FR5969
 *
 */
/* --COPYRIGHT--,BSD
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/


#include <msp430.h>
#include <stdint.h>
#include "TI_MSPBoot_Common.h"
#include "./Comm/TI_MSPBoot_CI.h"
#include "./MI/TI_MSPBoot_MI.h"
#include "./AppMgr/TI_MSPBoot_AppMgr.h"
#include "wisp-base.h"
#include "functions.h"
#include "puf.h"

//
//  Local function prototypes
//
static void HW_init(void);
static void MPU_init(void);

extern void CopyWispISRs(void);

// Helper data concatenated with challenge and nonce.
#pragma NOINIT (helper_data)
uint8_t helper_data[16 + 2*NONCE_ZS];

#pragma NOINIT (key)
uint8_t key[16];

#ifndef NO_SECURE

// Initialize the bootloader for firmware update. This function must be called early in the
// bootcycle to ensure that SRAM is not overwritten by static variables, or the stack
void secure_comms_init() {
    uint8_t ri[32];
    uint16_t nonce[NONCE_ZS];
    SRAMTRNG(nonce,NONCE_ZS);

    // Use the first byte of the nonce as the challenge
    uint8_t ci = (uint8_t)(nonce[0] & 0xFF);


    getPUF(ri, ci);              //request response ri from ci-th M-block

    encode_bch(&ri[0], &helper_data[0],&key[0]);
    sleeplpm3(SLEEPTIME);
    encode_bch(&ri[4], &helper_data[2],&key[2]);
    sleeplpm3(SLEEPTIME);
    encode_bch(&ri[8], &helper_data[4],&key[4]);
    sleeplpm3(SLEEPTIME);
    encode_bch(&ri[12], &helper_data[6],&key[6]);
    sleeplpm3(SLEEPTIME);
    encode_bch(&ri[16], &helper_data[8],&key[8]);
    sleeplpm3(SLEEPTIME);
    encode_bch(&ri[20], &helper_data[10],&key[10]);
    sleeplpm3(SLEEPTIME);
    encode_bch(&ri[24], &helper_data[12],&key[12]);
    sleeplpm3(SLEEPTIME);
    encode_bch(&ri[28], &helper_data[14],&key[14]);

    // Pack the challenge and the nonce at the end of the helper data
    unsigned int i;
    for (i = NONCE_ZS; i > 0; --i) {
        helper_data[16 + 2*NONCE_ZS - 2*i + 0] = (uint8_t)(nonce[NONCE_ZS - i] & 0xFF);
        helper_data[16 + 2*NONCE_ZS - 2*i + 1] = (uint8_t)(nonce[NONCE_ZS - i] >> 8);
    }
}

#endif


/******************************************************************************
 *
 * @brief   Main function
 *  - Initializes the MCU
 *  - Selects whether to run application or bootloader
 *  - If bootloader:
 *      - Initializes the peripheral interface
 *      - Waits for a command
 *      - Sends the corresponding response
 *  - If application:
 *      - Jump to application
 *
 * @return  none
 *****************************************************************************/
int main(void)
{
    // Stop watchdog timer to prevent time out reset
    WDTCTL = WDTPW + WDTHOLD;

    HW_init();

    tBOOL app_valid = TI_MSPBoot_AppMgr_ValidateApp();

	// Generate data for secure comms
    if (!app_valid) {
        CopyWispISRs();

#ifndef NO_SECURE
        secure_comms_init();
#endif
    }

    // Initialize WISP specific internals
    WISP_init();

    if (app_valid == TRUE_t) {
        // Copy app ISRs then setup MPU segements and lock the MPU registers
        CopyAppISRs();
        MPU_init();

        // Configure watchdog to trigger reset if the app doesn't load properly.
        WDTCTL = WDTPW + WDTCNTCL;
        TI_MSPBoot_APPMGR_JUMPTOAPP();
    }

    MPU_init();
    TI_MSPBoot_CI_Init();      // Initialize the Communication Interface

    while(1)
    {
        // Poll PHY and Data Link interface for new packets
        TI_MSPBoot_CI_PHYDL_Poll();

        // If a new packet is detected, process it
        if (TI_MSPBoot_CI_Process() == RET_JUMP_TO_APP)
        {
            // If Packet indicates a jump to App
            TI_MSPBoot_AppMgr_JumpToApp();
        }
    }
}


/******************************************************************************
 *
 * @brief   Initializes the basic MCU HW
 *
 * @return  none
 *****************************************************************************/
static void HW_init(void)
{
    // Just initialize S2 button to force BSL mode
    P1OUT |= BIT1;
    P1REN |= BIT1;
    PM5CTL0 &= ~LOCKLPM5;
}

/******************************************************************************
 *
 * @brief   Initializes the Memory Protection Unit of FR5969
 *          This allows for HW protection of Bootloader area
 *
 * @return  none
 *****************************************************************************/
static void MPU_init(void)
{
    // NOTE: segments must be paged aligned!

    // These calculations work for FR5969 (check user guide for MPUSEG values)
    //  Border 1 = Start of bootloader
    //  Border 2 = 0x10000
    //  Segment 1 = 0x4400 - Bootloader
    //  Segment 2 = Bootloader - 0xFFFF
    //  Segment 3 = 0x10000 - 0x23FFF
    //  Segment 2 is write protected and generates a PUC

    MPUCTL0 = MPUPW;                        // Write PWD to access MPU registers
    MPUSEGB1 = (BOOT_START_ADDR) >> 4;      // B1 = Start of Boot; B2 = 0x10000
    MPUSEGB2 = (0x10000) >> 4;
    MPUSAM &= ~MPUSEG2WE;                   // Segment 2 is protected from write
    MPUSAM |= MPUSEG2VS;                     // Violation select on write access
    MPUCTL0 = MPUPW | MPUENA;                 // Enable MPU protection
    MPUCTL0_H = 0x00;                         // Disable access to MPU registers
}
