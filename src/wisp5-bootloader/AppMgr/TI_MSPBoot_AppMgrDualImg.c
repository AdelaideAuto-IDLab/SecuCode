/*
 * \file   TI_MSPBoot_AppMgr.c
 *
 * \brief  Application Manager. Handles App validation, decides if device
 *         should jump to App or stay in bootloader
 *         This file supports dual image
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

//
// Include files
//
#include "msp430.h"
#include "TI_MSPBoot_Common.h"
#include "TI_MSPBoot_AppMgr.h"
#include "crc.h"
#include "wisp-base.h"
#include "functions.h"

//#define USE_LPM35_RESET
//#define FORCE_POWER_LOSS


//
//  Global variables
//
/*! Password sent by Application to force boot mode. This variable is in a fixed
    location and should keep same functionality and location in Boot and App */
#ifdef __IAR_SYSTEMS_ICC__
#           pragma location="RAM_PASSWORD"
    __no_init uint16_t  PassWd;
#       elif defined (__TI_COMPILER_VERSION__)
extern uint16_t  PassWd;
#endif

/*! Status and Control byte. This variable is in a fixed
 location and should keep same functionality and location in Boot and App */
#ifdef __IAR_SYSTEMS_ICC__
#           pragma location="RAM_STATCTRL"
    __no_init uint8_t  StatCtrl;
#       elif defined (__TI_COMPILER_VERSION__)
extern uint8_t  StatCtrl;
#endif

//
//  Local function prototypes
//
static tBOOL TI_MSPBoot_AppMgr_BootisForced(void);

/******************************************************************************
 *
 * @brief   Checks if an Application is valid
 * @return  TRUE_t if application is valid,
 *          FALSE_t if application is invalid
 *****************************************************************************/
static tBOOL TI_MSPBoot_AppMgr_AppisValid()
{
    extern uint16_t _Appl_Reset_Vector;
    return *(volatile uint16_t *)(&_Appl_Reset_Vector) != 0xFFFF ? TRUE_t : FALSE_t;
}

/******************************************************************************
 *
 * @brief   Decides whether to stay in MSPBoot or if it should jump to App
 *  MSPBoot:  Boot mode is forced by a call from App, OR
 *          Boot mode is forced by an external event (button pressed), OR
 *          Application is invalid
 *  App:    Boot mode is not forced, AND
 *          Application is valid
 *
 * @return  TRUE_t if application is valid and should be executed
 *          FALSE_t if we must stay in Boot mode
 *****************************************************************************/
tBOOL TI_MSPBoot_AppMgr_ValidateApp(void)
{
#ifdef BENCHMARK
    return FALSE_t;
#endif

    if (TI_MSPBoot_AppMgr_BootisForced() == FALSE_t)
    {
        return TI_MSPBoot_AppMgr_AppisValid();
    }

    // StatCtrl = 0;
    // PassWd = 0;

    return FALSE_t;
}


void TI_MSPBoot_AppMgr_JumpToApp(void)
{
#ifdef BENCHMARK
    extern WISP_dataStructInterface_t wispData;
    wispData.epcBuf[2] = 0x88;
    TI_MSPBoot_AppMgr_RestartInBootMode();
    return;
#endif

    StatCtrl = 0;
    PassWd = 0;

    // Force a PUC reset by writing incorrect Watchdog password
    // (this is generally faster than triggering a full reset)
    WDTCTL = 0xDEAD;
}

void TI_MSPBoot_AppMgr_RestartInBootMode(void) {

    StatCtrl |= BOOT_APP_REQ;
    PassWd = BSL_PASSWORD;

#ifdef FORCE_POWER_LOSS
    // Try to force power loss by busy waiting with the LED on
    PLED1OUT |= PIN_LED1;
    while(TRUE) {}
#endif

#ifdef USE_LPM35_RESET
    // Try to force SRAM state to be lost by entering LPM3.5
    PMMCTL0_H = PMMPW_H; // Open PMM Registers for write
    PMMCTL0_L |= PMMREGOFF; // and set PMMREGOFF
    PMMCTL0_H = 0; // Lock PMM Registers

    sleeplpm3(100);

    PMMCTL0 = PMMPW | PMMSWBOR;
#endif

    // Force a PUC reset by writing incorrect Watchdog password
    WDTCTL = 0xDEAD;
}


/******************************************************************************
 *
 * @brief   Checks if Boot mode is forced
 *          Boot mode is forced by an application call sending a request and password
 *          Or by an external event such as a button press
 *
 * @return  TRUE_t Boot mode is forced
 *          FALSE_t Boot mode is not forced
 *****************************************************************************/
static tBOOL TI_MSPBoot_AppMgr_BootisForced(void)
{
    // Check if application is requesting Boot mode and password is correct
    if (((StatCtrl & BOOT_APP_REQ) != 0) && (PassWd == BSL_PASSWORD))
    {
        return TRUE_t;
    }

    // Check if we were reset by the watchdog timer
    if ((SFRIFG1 & WDTIFG) == WDTIFG)
    {
        return TRUE_t;
    }

    // Check for an external event such as S3 (P1.3) button in Launchpad
    // __delay_cycles(10000);   // Wait for pull-up to go high
    //If S2 button (P1.3) is pressed, force BSL
    if (HW_ENTRY_CONDITION)
    {
        return TRUE_t;
    }

    return FALSE_t;
}

