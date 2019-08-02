/*
 * \file   TI_MSPBoot_CI_PHYDL.h
 *
 * \brief  Header file the Physical-DataLink layer of the communication interface
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

#ifndef __TI_MSPBoot_CI_PHYDL_H__
#define __TI_MSPBoot_CI_PHYDL_H__

// 
// Type definitions
//
/*! Structure defining the callback functions implemented in the communication
    interface.
    For example, when a RX flag is detected by the PHY, it will call the 
    callback function to inform the NWK layer.
*/
typedef struct  {
    void (*RxCallback)(uint8_t);            // RX Flag callback
    void (*TxCallback)(uint8_t *);          // TX Flag callback
    void (*ErrorCallback)(uint8_t);         // Error callback
}t_CI_Callback;


//
// Function prototypes
//
extern void TI_MSPBoot_CI_PHYDL_Init(t_CI_Callback * CI_Callback);
extern void TI_MSPBoot_CI_PHYDL_Poll(void);
extern void TI_MSPBoot_CI_PHYDL_TXByte(uint8_t byte);

#endif //__TI_MSPBoot_CI_PHYDL_H__
