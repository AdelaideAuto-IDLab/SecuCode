/*
 * \file   TI_MSPBoot_Config.h
 *
 * \brief  Contains definitions used to configure the bootloader for FR6989
 *         supporting the Simple, BSL-Based, SMBus configurations using UART or CC1101
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

#ifndef __TI_MSPBoot_CONFIG_H__
#define __TI_MSPBoot_CONFIG_H__

//
// Include files
//
#define MCLK                    (8000000L)  /*! MCLK Frequency in Hz */
#define MSPBoot_BSL

/*! Watchdog feed sequence */
#define WATCHDOG_FEED()         {WDTCTL = (WDTPW+WDTCNTCL+WDTSSEL__VLO+WDTIS__8192);}
/*! Hardware entry condition in order to force bootloader mode */
#define HW_ENTRY_CONDITION      (0)
/*! HW_RESET_BOR: Force a software BOR in order to reset MCU 
     Not all MCUs support this funcitonality. Check datasheet/UG for details.
    HW_RESET_PUC: Force a PUC in order to reset MCU 
     An invalid write to watchdog will force the PUC.
 */
//#define HW_RESET_BOR

//
// Configuration MACROS
//
/* MSPBoot_SIMPLE :
 *  If MSPBoot_SIMPLE is defined for the project (in this file or in project 
 *    preprocessor options), the following settings will apply:
 *
 *  NDEBUG = defined: Debugging is disabled, ASSERT_H function is ignored,
 *      GPIOs are not used for debugging
 *
 *  CONFIG_APPMGR_APP_VALIDATE =1: Application is validated by checking its reset
 *      vector. If the vector is != 0xFFFF, the application is valid
 *
 *  CONFIG_MI_MEMORY_RANGE_CHECK = undefined: Address range of erase and Write
 *      operations are not validated. The algorithm used by SimpleProtocol
 *      doesn't write to invalid areas anyways
 *
 *  CONFIG_CI_PHYDL_COMM_SHARED = undefined: Communication interface is not used 
 *      by application. Application can still initialize and use same interface 
 *      on its own
 *
 *  CONFIG_CI_PHYDL_ERROR_CALLBACK = undefined: The communication interface  
 *      doesn't report errors with a callback
 *
 *  CONFIG_CI_PHYDL_I2C_TIMEOUT = undefined: The communication interface doesn't 
 *      detect timeouts. Only used with I2C
 *
 *  CONFIG_CI_PHYDL_START_CALLBACK = undefined: Start Callback is not needed
 *      and is not implemented to save flash space. Only used with I2C
 *
 *  CONFIG_CI_PHYDL_STOP_CALLBACK = undefined: Stop callback is not needed
 *      and is not implemented to save flash space. Only used with I2C
 *
 *  CONFIG_CI_PHYDL_I2C_SLAVE_ADDR = 0x40. I2C slave address of this device 
 *      is 0x40
 *
 *  CONFIG_CI_PHYDL_UART_BAUDRATE = 9600. UART baudrate is 9600. Only used with
 *      UART
 */
#if defined(MSPBoot_SIMPLE)
//#   define NDEBUG
#   define CONFIG_APPMGR_APP_VALIDATE    (1)
#   undef CONFIG_MI_MEMORY_RANGE_CHECK
#   undef CONFIG_CI_PHYDL_COMM_SHARED
#   undef CONFIG_CI_PHYDL_ERROR_CALLBACK
#   undef CONFIG_CI_PHYDL_I2C_TIMEOUT
#   ifdef MSPBoot_CI_UART
#       undef CONFIG_CI_PHYDL_START_CALLBACK
#       undef CONFIG_CI_PHYDL_STOP_CALLBACK
#       define CONFIG_CI_PHYDL_UART_BAUDRATE        (9600)
#   endif
   

/* MSPBoot BSL-based:
 *  If MSPBoot_BSL is defined for the project (in this file or in project 
 *  preprocessor options), the following settings will apply:
 *
 *  NDEBUG = defined: Debugging is disabled, ASSERT_H function is ignored,
 *      GPIOs are not used for debugging
 *
 *  CONFIG_APPMGR_APP_VALIDATE =2: Application is validated by checking its CRC-16.
 *      An invalid CRC over the whole Application area will keep the mcu in MSPBoot
 *
 *  CONFIG_MI_MEMORY_RANGE_CHECK = defined: Address range of erase and Write
 *      operations are validated. BSL-based commands can write/erase any area
 *      including MSPBoot, but this defition prevents modifications to area 
 *      outside of Application
 *
 *  CONFIG_CI_PHYDL_COMM_SHARED = defined: Communication interface can be used 
 *      by application. Application can call MSPBoot initialization and poll 
 *      routines to use the same interface.
 *
 *  CONFIG_CI_PHYDL_ERROR_CALLBACK = undefined: The communication interface  
 *      doesn't report errors with a callback
 *
 *  CONFIG_CI_PHYDL_I2C_TIMEOUT = undefined: The communication interface doesn't 
 *      detect timeouts. Only used with I2C
 *
 *  CONFIG_CI_PHYDL_START_CALLBACK = defined: Start Callback is required
 *      by the BSL-based protocol and is implemented. Only used with I2C.
 *
 *  CONFIG_CI_PHYDL_STOP_CALLBACK = undefined: Stop callback is not needed
 *      and is not implemented to save flash space. Only used with I2C
 *
 *  CONFIG_CI_PHYDL_I2C_SLAVE_ADDR = 0x40. I2C slave address of this device 
 *      is 0x40. Only used with I2C
 *
 *  CONFIG_CI_PHYDL_UART_BAUDRATE = 9600. UART baudrate is 9600. Only used with
 *      UART
 */
#elif defined(MSPBoot_BSL)
//#   define NDEBUG
#   define CONFIG_APPMGR_APP_VALIDATE    (2)
#   define CONFIG_MI_MEMORY_RANGE_CHECK
#   define CONFIG_CI_PHYDL_COMM_SHARED
#   undef CONFIG_CI_PHYDL_ERROR_CALLBACK
#   ifdef MSPBoot_CI_UART
#       undef CONFIG_CI_PHYDL_START_CALLBACK
#       undef CONFIG_CI_PHYDL_STOP_CALLBACK
#       define CONFIG_CI_PHYDL_UART_BAUDRATE        (115200)
#	endif
#	ifdef MSPBoot_CI_CC1101
#       undef CONFIG_CI_PHYDL_START_CALLBACK
#       undef CONFIG_CI_PHYDL_STOP_CALLBACK
#       define CONFIG_CI_PHYDL_CC1101_FREQUENCY     (902750)
#   endif
#else
#error "Define a proper configuration in TI_MSPBoot_Config.h or in project preprocessor options"
#endif

#endif            //__TI_MSPBoot_CONFIG_H__
