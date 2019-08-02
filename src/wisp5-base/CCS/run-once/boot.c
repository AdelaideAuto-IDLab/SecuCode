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
/*****************************************************************************/
/* BOOT.C   v4.1.0 - Initialize the MSP430 C runtime environment             */
/*                                                                           */
/* Copyright (c) 2003-2012 Texas Instruments Incorporated                    */
/* http://www.ti.com/                                                        */
/*                                                                           */
/*  Redistribution and  use in source  and binary forms, with  or without    */
/*  modification,  are permitted provided  that the  following conditions    */
/*  are met:                                                                 */
/*                                                                           */
/*     Redistributions  of source  code must  retain the  above copyright    */
/*     notice, this list of conditions and the following disclaimer.         */
/*                                                                           */
/*     Redistributions in binary form  must reproduce the above copyright    */
/*     notice, this  list of conditions  and the following  disclaimer in    */
/*     the  documentation  and/or   other  materials  provided  with  the    */
/*     distribution.                                                         */
/*                                                                           */
/*     Neither the  name of Texas Instruments Incorporated  nor the names    */
/*     of its  contributors may  be used to  endorse or  promote products    */
/*     derived  from   this  software  without   specific  prior  written    */
/*     permission.                                                           */
/*                                                                           */
/*  THIS SOFTWARE  IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS    */
/*  "AS IS"  AND ANY  EXPRESS OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT    */
/*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR    */
/*  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT    */
/*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    */
/*  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL  DAMAGES  (INCLUDING, BUT  NOT    */
/*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,    */
/*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY    */
/*  THEORY OF  LIABILITY, WHETHER IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE    */
/*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.     */
/*                                                                           */
/*****************************************************************************/
#include <stdlib.h>

extern void main(void);
extern void __interrupt _c_int00();


void              (*_cleanup_ptr)(void);
void _DATA_ACCESS (*_dtors_ptr)(int);

/*---------------------------------------------------------------------------*/
/* Allocate the memory for the system stack.  This section will be sized     */
/* by the linker.                                                            */
/*---------------------------------------------------------------------------*/
__asm("\t.global __STACK_END");
#pragma DATA_SECTION (_stack, ".stack");
#if defined(__LARGE_DATA_MODEL__)
long _stack;
#else
int _stack;
#endif

/*---------------------------------------------------------------------------*/
/*  Initialize reset vector to point at _c_int00                             */
/*  _c_int00 must always be located in low-memory on MSP430X devices.        */
/*---------------------------------------------------------------------------*/
#if defined(__LARGE_CODE_MODEL__)
_Pragma("CODE_SECTION(_c_int00, \".text:_isr\")")
#endif

__asm("\t.global _reset_vector");
__asm("\t.sect   \".reset\"");
__asm("\t.align  2");
__asm("_reset_vector:\n\t.field _c_int00, 16");

/*---------------------------------------------------------------------------*/
/* Macro to initialize stack pointer.  Stack grows towards lower memory.     */
/*---------------------------------------------------------------------------*/
#if defined(__LARGE_DATA_MODEL__)
#define STACK_INIT() __asm("\t   MOVX.A\t   #__STACK_END,SP")
#else
#define STACK_INIT() __asm("\t   MOV.W\t    #__STACK_END,SP")
#endif

/*---------------------------------------------------------------------------*/
/* Macros to initialize required global variables.                           */
/*---------------------------------------------------------------------------*/
#if defined(__TI_EABI__)
#define INIT_EXIT_PTRS() do { } while(0)
#define INIT_LOCKS()     do { } while(0)
#else
#define INIT_EXIT_PTRS() do { _cleanup_ptr = NULL; _dtors_ptr = NULL; } while(0)
#define INIT_LOCKS()     do { _lock = _nop; _unlock = _nop; } while(0)
#endif

/*****************************************************************************/
/* C_INT00() - C ENVIRONMENT ENTRY POINT                                     */
/*****************************************************************************/
#pragma CLINK(_c_int00)
extern void __interrupt _c_int00()
{
   STACK_INIT();

 //  INIT_EXIT_PTRS();
  // INIT_LOCKS();

   /*------------------------------------------------------------------------*/
   /* Allow for any application-specific low level initialization prior to   */
   /* initializing the C/C++ environment (global variable initialization,    */
   /* constructers).  If _system_pre_init() returns 0, then bypass C/C++     */
   /* initialization.  NOTE: BYPASSING THE CALL TO THE C/C++ INITIALIZATION  */
   /* ROUTINE MAY RESULT IN PROGRAM FAILURE.                                 */
   /*------------------------------------------------------------------------*/
//   if(_system_pre_init() != 0)  _auto_init();

   /*------------------------------------------------------------------------*/
   /* Handle any argc/argv arguments if supported by an MSP430 loader.       */
   /*------------------------------------------------------------------------*/
   main();
}
