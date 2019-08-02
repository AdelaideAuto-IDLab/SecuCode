/*
 * \file   CRC.h
 *
 * \brief  Driver for CRC-8/CRC-CCITT 
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

/*! Polynomial for CRC8 algorithm */
#define CRC8_POLY   0x07
/*! Polynomial for CRC-CCITT algorithm */
#define CRC16_POLY   0x1021

//
//  Function declarations
//
#ifndef __MSP430_HAS_CRC__
/******************************************************************************
 *
 * @brief   CRC_CCITT as implemented in slaa221
 *          This implementation is slower but smaller than table method
 *
 * @param pmsg  Pointer to data being calculated
 * @param msg_size  Size of data being calculated
 *
 * @return  16-bit CRC_CCITT result
 *****************************************************************************/
uint16_t crc16MakeBitwise(uint8_t *pmsg, uint16_t msg_size)
{
    uint16_t i, j;
    uint16_t msg;
    uint16_t crc = 0xFFFF;

    for(i = 0 ; i < msg_size ; i ++)
    {
        msg = (*pmsg++ << 8);

		for(j = 0 ; j < 8 ; j++)
        {
            if((msg ^ crc) >> 15) crc = (crc << 1) ^ CRC16_POLY;
			else crc <<= 1;	
			msg <<= 1;
        }
    }

    return(crc);
}
#else
/******************************************************************************
 *
 * @brief   CRC_CCITT using hardware module
 *
 * @param pmsg  Pointer to data being calculated
 * @param msg_size  Size of data being calculated
 *
 * @return  16-bit CRC_CCITT result
 *****************************************************************************/
uint16_t crc16MakeBitwise(uint8_t *pmsg, uint32_t msg_size)
{
    uint32_t i;
    
    CRCINIRES = 0xFFFF;
    for(i = 0 ; i < msg_size ; i ++)
    {
        CRCDIRB_L = *pmsg++;
    }
    return(CRCINIRES);
}
#endif

/******************************************************************************
 *
 * @brief   Add a byte to CRC8 calculation
 *
 * @param crc       pointer to current crc value, updated with the new crc
 * @param new_data  byte added to the crc
 *
 * @return  none
 *****************************************************************************/
void crc8_add(uint8_t *crc, uint8_t new_data)
{
    uint8_t i;	// Counter for 8 shifts

    *crc ^= new_data;        // Initial XOR

    i = 8;
    do
    {
        if (*crc & 0x80)
        {
            *crc <<= 1;
            *crc ^= CRC8_POLY;
        }
        else
        {
            *crc <<= 1;
        }
    }
    while(--i);
}


/******************************************************************************
 *
 * @brief   Calculates CRC8 of a packet
 *
 * @param pmsg  Pointer to data being calculated
 * @param msg_size  Size of data being calculated
 *
 * @return  8-bit CRC8 result
 *****************************************************************************/
uint8_t crc8MakeBitwise(uint8_t *pmsg, uint16_t msg_size)
{
    uint16_t i;
    uint8_t crc = 0x00;

    for(i = 0 ; i < msg_size ; i ++)
    {
        crc8_add(&crc, *pmsg++);
    }

    return(crc);
}
