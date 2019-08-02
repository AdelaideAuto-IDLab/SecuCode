/*
 * \file   TI_MSPBoot_AppMgr.h
 *
 * \brief  Header file of Application Manager
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

#ifndef __TI_MSPBoot_APPMGR_H__
#define __TI_MSPBoot_APPMGR_H__

//
//   External variables from linker file
//
extern uint16_t _Appl_Reset_Vector;   /*! Application Reset vector address */


//
// Macros and Definitions
//
/*! Bit in StatCtrl used by application to force boot mode */
#define BOOT_APP_REQ        BIT0
/*! Magic Key sent by Application to force boot mode */
#define BSL_PASSWORD        (0xC0DE)
/*! Jumps to application using its reset vector address */
#define TI_MSPBoot_APPMGR_JUMPTOAPP()     {((void (*)()) _Appl_Reset_Vector) ();}

//
//  Function prototypes
//
extern tBOOL TI_MSPBoot_AppMgr_ValidateApp(void);
extern void TI_MSPBoot_AppMgr_JumpToApp(void);

#endif //__TI_MSPBoot_APPMGR_H__
