/*
 * \file   TI_MSPBoot_Common.h
 *
 * \brief  Header with Common definitions used by the project
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

#ifndef __TI_MSPBoot_COMMON_H__
#define __TI_MSPBoot_COMMON_H__

//
// Include files
//
#include <stdint.h>
#include "TI_MSPBoot_Config.h"

//
// Type definitions
//
/*! Boolean type definition */
typedef enum
{
    FALSE_t=0,
    TRUE_t
} tBOOL;

//
// MACROS
//
/*! Used for debugging purposes. This macro valuates an expression when NDEBUG
    is not defined and if true, it stays in an infinite loop */
#ifndef ASSERT_H
    #ifndef NDEBUG
        #define ASSERT_H(expr) \
                if (!expr) {\
                 while (1);\
                }
    #else
        #define ASSERT_H(expr)
    #endif
#endif

#define NULL    0x00            /*! NULL definition */

                
//
// Function return values
//
#define RET_OK              0   /*! Function returned OK */
#define RET_PARAM_ERROR     1   /*! Parameters are incorrect */
#define RET_JUMP_TO_APP     2   /*! Function exits and jump to application */


#endif //__TI_MSPBoot_COMMON_H__
