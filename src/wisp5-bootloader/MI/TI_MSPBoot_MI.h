/*
 * \file   TI_MSPBoot_MI.h
 *
 * \brief  Header file for the Memory Interface 
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

#ifndef __TI_MSPBoot_MI_H__
#define __TI_MSPBoot_MI_H__

//
// Include files
//

//
//  External variables from linker file. 
//  Note that this file gets constants from linker file in order to be able
//  to check where the Application resides, avoid corrupting boot area, etc
//
extern uint16_t _Appl_End;                  /*! Application End Address */
extern uint32_t _Flex_Start;           		/*! Flex Area Start Address */
extern uint32_t _Flex_End;             		/*! Flex Area End Address */
extern uint16_t _Appl_Checksum;             /*! Application Checksum Address */
extern uint16_t _Appl_Proxy_Vector_Start;   /*! Application Vector Table Start */
extern uint16_t _Appl_Reset_Vector;         /*! Application Reset vector */
extern uint16_t __Boot_VectorTable;         /*! Bootloader Vector Table Start */
extern uint16_t __Boot_Start;				/*! Bootloader Start */

//
//  Macros and definitions
//
/*! Application start address (from linker file) */
#define APP_START_ADDR          ((uint32_t )&_Appl_Checksum)
/*! Application end address (from linker file) */
#define APP_END_ADDR            ((uint32_t )&_Appl_End)
/*! Flex area start address (from linker file) */
#define FLEX_START_ADDR          ((uint32_t )&_Flex_Start)
/*! Flex area end address (from linker file) */
#define FLEX_END_ADDR            ((uint32_t )&_Flex_End)
/*! Application Vector Table */
#define APP_VECTOR_TABLE        ((uint32_t) &_Appl_Proxy_Vector_Start)
/*! Application Reset Vector */
#define APP_RESET_VECTOR_ADDR   ((uint32_t) &_Appl_Reset_Vector)
/*! Application Interrupt Table (from linker file) */
#define BOOT_VECTOR_TABLE       ((uint32_t) &__Boot_VectorTable)
/*! Boot start address (from linker file) */
#define BOOT_START_ADDR       ((uint32_t) &__Boot_Start)

//
//  Functions prototypes
//
extern uint8_t TI_MSPBoot_MI_EraseSector(uint32_t addr);
extern void TI_MSPBoot_MI_EraseApp(void);
extern uint8_t TI_MSPBoot_MI_WriteByte(uint32_t addr, uint8_t data);

extern void CopyAppISRs(void);

#endif //__TI_MSPBoot_MI_H__

