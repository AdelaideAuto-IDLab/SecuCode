/*
 * \file   TI_MSPBoot_MI_FRAMDualImg.c
 *
 * \brief  Driver for memory interface using FRAM in FR59xx
 *         This file supports Dual Image
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
#include "TI_MSPBoot_MI.h"

//
// Local Function prototypes
//
static uint8_t TI_MSPBoot_MI_EraseSectorDirect(uint32_t addr);
static uint8_t TI_MSPBoot_MI_WriteByteDirect(uint32_t addr, uint8_t data);
// Function prototypes
void TI_MSPBoot_MI_EraseAppDirect(uint8_t DownArea);
void TI_MSPBoot_MI_ReplaceApp(void);
uint32_t TI_MSPBoot_MI_GetPhysicalAddressFromVirtual(uint32_t addr);

//
//  Functions declarations
//
/******************************************************************************
*
 * @brief   Erase a FRAM Sector
 *          FRAM doesn't have sectors or an "erase" state but this function
 *          is added for compatibility with Flash and erases as 0xFF
 *          in order to calculate CRC
 *
 * @param  addr    Address in the sector being erased (sector is 512B)
 *                  The actual address will be in Download area
 *
 * @return  RET_OK when sucessful,
 *          RET_PARAM_ERROR if address is outside of Application area
 *****************************************************************************/
uint8_t TI_MSPBoot_MI_EraseSector(uint32_t addr)
{
#ifdef CONFIG_MI_MEMORY_RANGE_CHECK
    // Check address to be within Application range
	if ((addr < APP_START_ADDR) || (addr > APP_END_ADDR) && (addr < FLEX_START_ADDR) || (addr > FLEX_END_ADDR))
        return RET_PARAM_ERROR;
#endif

    // Erase the corresponding area
    TI_MSPBoot_MI_EraseSectorDirect(TI_MSPBoot_MI_GetPhysicalAddressFromVirtual(addr));

    return RET_OK;
}

/******************************************************************************
*
 * @brief   Erase a FRAM Sector directly
 *
 * @param  addr    Address in the sector being erased (sector is 512B)
 *          FRAM doesn't have sectors or an "erase" state but this function
 *          is added for compatibility with Flash and erases as 0xFF
 *          in order to calculate CRC
 *
 * @return  RET_OK when sucessful,
 *          RET_PARAM_ERROR if address is outside of Application area
 *****************************************************************************/
static uint8_t TI_MSPBoot_MI_EraseSectorDirect(uint32_t addr)
{
    uint32_t i;
    // Erase is not necessary for FRAM but we do it in order to calculate CRC
    // properly
    for (i = addr; i < (addr+512); i+=2)
    {
        __data20_write_short(i, 0xFFFF);    // Write 256words= 512 bytes
    }


    return RET_OK;
}

/******************************************************************************
 *
 * @brief   Erase the application area (address obtained from linker file)
 *          Erases the application in Download Area
 *          FRAM doesn't have an "erased" state but this function is added
 *          for compatibility with Flash and in order to calculate CRC
 *
 * @return  none
 *****************************************************************************/
void TI_MSPBoot_MI_EraseApp(void)
{
    // Erase Download Area
    TI_MSPBoot_MI_EraseAppDirect(1);

}

/******************************************************************************
 *
 * @brief   Erase the application area (address obtained from linker file)
 *          It can erase application in Download or App Area
 *          FRAM doesn't have an "erased" state but this function is added
 *          for compatibility with Flash and in order to calculate CRC
 *
 * @param DownArea 1:Erase Download area, 0: Erase App Area
 *
 * @return  none
 *****************************************************************************/
void TI_MSPBoot_MI_EraseAppDirect(uint8_t DownArea)
{
    uint32_t addr;

    for (addr = APP_START_ADDR; addr <= APP_END_ADDR; addr+=2)
    {
        if (DownArea==0)
        {
            // Erase all the app area
            __data20_write_short(addr, 0xFFFF);
        }
        else
        {
            // Erase the download area
            __data20_write_short(TI_MSPBoot_MI_GetPhysicalAddressFromVirtual(addr), 0xFFFF);
        }
    }

    if(FLEX_START_ADDR == 0x10000)
    {
		for (addr = FLEX_START_ADDR; addr <= FLEX_END_ADDR; addr+=2)
		{
			if (DownArea==0)
			{
				// Erase all the app area
				__data20_write_short(addr, 0xFFFF);
			}
			else
			{
				// Erase the download area
				__data20_write_short(TI_MSPBoot_MI_GetPhysicalAddressFromVirtual(addr), 0xFFFF);
			}
		}
    }

}

/******************************************************************************
 *
 * @brief   Write a Byte to FRAM memory
 *      This function writes the byte to Download area
 *
 * @param  addr     Address of the Byte being written in Application area
 *                  The actual address will be in Download area
 * @param  data     Byte being written
 *
 * @return  RET_OK when sucessful,
 *          RET_PARAM_ERROR if address is outside of Application area
 *****************************************************************************/
uint8_t TI_MSPBoot_MI_WriteByte(uint32_t addr, uint8_t data)
{
#ifdef CONFIG_MI_MEMORY_RANGE_CHECK
    // Check address to be within Application range
	if ((addr < APP_START_ADDR) || (addr > APP_END_ADDR) && (addr < FLEX_START_ADDR) || (addr > FLEX_END_ADDR))
        return RET_PARAM_ERROR;
#endif

    // Write the byte
    TI_MSPBoot_MI_WriteByteDirect(addr, data);

    return RET_OK;
}


/******************************************************************************
 *
 * @brief   Write a Byte Directly to Flash memory
 *          The bootloader is protected using MPU but all interrupts (except for
 *          Vector) can be reprogrammed
 *
 * @param  addr     Address of the Byte being written in Flash
 * @param  data     Byte being written
 *
 * @return  RET_OK when sucessful,
 *          RET_PARAM_ERROR if address is outside of Application area
 *****************************************************************************/
static uint8_t TI_MSPBoot_MI_WriteByteDirect(uint32_t addr, uint8_t data)
{
    __data20_write_char(addr, data);    // Write to memory

    // Since we need to make use of the interrupts as part of the RFID stack we skip updating
    // them here and instead update them as part of the boot process.
    //if ((addr >= APP_VECTOR_TABLE) && (addr < APP_RESET_VECTOR_ADDR - 2))
    //{
    //    return RET_OK;
    //}

    return RET_OK;
}

// Copy the app IVT into the device IVT. This function will not work if it is called after the MPU
// is locked
void CopyAppISRs(void) {
    MPUCTL0 = MPUPW | MPUENA;   // Enable access to MPU registers
    MPUSAM |= MPUSEG2WE;        // Enable Write access

    uint32_t addr;
    for (addr = APP_VECTOR_TABLE; addr < APP_RESET_VECTOR_ADDR - 2; ++addr) {
        uint32_t target = (addr - APP_VECTOR_TABLE) + BOOT_VECTOR_TABLE;
        __data20_write_char(target, *((uint8_t*)addr));
    }

    MPUSAM &= ~MPUSEG2WE;       // Disable Write access
    MPUCTL0_H = 0x00;           // Disable access to MPU registers
}


/******************************************************************************
 *
 * @brief   Replaces the application area with the contents of download area
 *          This function should only be called after validating the download
 *          area.
 *
 * @return  none
 *****************************************************************************/
void TI_MSPBoot_MI_ReplaceApp(void)
{
    volatile uint32_t addr;

    for (addr = APP_START_ADDR; addr <= APP_END_ADDR; addr++)
    {
        TI_MSPBoot_MI_WriteByteDirect(addr, __data20_read_char(TI_MSPBoot_MI_GetPhysicalAddressFromVirtual(addr)));
        __no_operation();
    }

    if(FLEX_START_ADDR == 0x10000)
    {
        for (addr = FLEX_START_ADDR; addr <= FLEX_END_ADDR; addr++)
        {
        	TI_MSPBoot_MI_WriteByteDirect(addr, __data20_read_char(TI_MSPBoot_MI_GetPhysicalAddressFromVirtual(addr)));
        	__no_operation();
        }
    }
}


/******************************************************************************
 *
 * @brief   Convert a virtual address of application to a physical address in
 *          download area
 *
 * @param  addr     Address in appplication memory
 *
 * @return  Physical address in download area
 *
 * @note: the address must be in application area, this function doesn't validate
 *          the input
 *****************************************************************************/
uint32_t TI_MSPBoot_MI_GetPhysicalAddressFromVirtual(uint32_t addr)
{
    volatile uint32_t ret;
    volatile uint32_t address;
    extern uint32_t _Down_Offset_Size;			/*! Download Offset Size */
    extern uint32_t _Down_Offset1;				/*! Download Offset 1 */
    extern uint32_t _Down_Offset2;				/*! Download Offset 2 */

    // Application will be downloaded to 2 areas in memory. I.e.
    // data from 4400-7DFF  will be downloaded to BE00-F7FF
    // data from 7E00-BDFF will be downloaded to 10000-13FFF

    address = addr;

    // If the address-offset fits in 1st app area
    if (address <=  (uint32_t )&_Down_Offset_Size)
	{
		ret = (addr + (uint32_t )&_Down_Offset1);
	}
	else    // Else, place it in the 2nd download area
	{
		ret = (addr + (uint32_t )&_Down_Offset2);
	}


    return ret;
}
