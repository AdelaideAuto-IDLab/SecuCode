;/**@file		rfid_BlockWriteHandle.asm
;*	@brief
;* 	@details
;*
;*	@author		Jethro Tan, Aaron Parks, Justin Reina, UW Sensor Systems Lab
;*	@created
;*	@last rev
;*
;*	@notes		In a blockwrite, data is transmitted in the clear (not cover coded by RN16)
;*				BLOCKWRITE: {CMD [8], MEMBANK [2], WordPtr [6?], WordCount [8], Data [VAR], RN [16], CRC [16]}
;*
;*	@section
;*/

    .cdecls C,LIST
%{
	#include "../globals.h"
	#include "../Math/crc16.h"
	#include "rfid.h"
	#include "../internals/authenticate.h"
%}

R_bits      .set  R5										;received data bit counter
R_wait		.set  R11
R_scratch3	.set  R12
R_cmd_ptr	.set  R12
R_scratch2	.set  R13
R_scratch1	.set  R14
R_scratch0	.set  R15

	.ref cmd
	.def  handleBlockWrite
	.global RxClock, TxClock
	.sect ".text"

handleBlockWrite:

;Wait for first two bytes to come in. then memBank is in cmd[1].b7b6
	MOV 	#16, R_wait
waitOnBits_0:
	CMP     R_wait, R_bits                                  ;[2] Proceed when R_bits > 16 (ceil 8+2 -> 16)
	JLO     waitOnBits_0                                    ;[2]

calc_memBank:
	MOV.B	(cmd+1), R_scratch0                             ;[3] load cmd byte 2 into R_scratch0. memBank is in b7b6 (0xC0)
	AND.B	#0xC0, R_scratch0                               ;[2] mask of non-memBank bits, then switch on it to load corr memBankPtr
	RRA		R_scratch0                                      ;[1] move b7b6 down to b1b0
	RRA		R_scratch0                                      ;[1] move b7b6 down to b1b0
	RRA		R_scratch0                                      ;[1] move b7b6 down to b1b0
	RRA		R_scratch0                                      ;[1] move b7b6 down to b1b0
	RRA		R_scratch0                                      ;[1] move b7b6 down to b1b0
	RRA		R_scratch0                                      ;[1] move b7b6 down to b1b0
	MOV.B   R_scratch0, &(RWData.memBank)                   ;[3] store the memBank

; Now wait until we have all bits to extract the WordPtr.
	ADD 	#16, R_wait										;[2]
	MOVA	#cmd, R_scratch3
waitOnBits_1:
	CMP.W   R_wait, R_bits                                  ;[2] Wait until first 3 bytes are fully received.
	JLO     waitOnBits_1                                    ;[2]

; Extract WordPtr.
; TODO: All this shifting around is a mess, it would be nice if we could just force the EBV values
; 		to byte aligned (e.g. by changing the ISR that process the bits) -- This would also mean the
;		actual data would also be byte aligned saving many instructions.
calc_wordPtr:
	MOV.B 	1(R_cmd_ptr), R_scratch0                        ;[3] bring in top 6 bits into b5-b0 of R_scratch0 (wordCt.b7-b2)
	MOV.B 	2(R_cmd_ptr), R_scratch1                        ;[3] bring in bot 2 bits into b7b6  of R_scratch1 (wordCt.b1-b0)

	RLC.B	R_scratch1                                      ;[1] pull out b7 from R_scratch1 (wordCt.b1)
	RLC.B	R_scratch0                                      ;[1] shove it into R_scratch0 at bottom (wordCt.b1)
	RLC.B	R_scratch1                                      ;[1] pull out b7 from R_scratch1 (wordCt.b0)
	RLC.B	R_scratch0                                      ;[1] shove it into R_scratch0 at bottom (wordCt.b0)

	MOV.B	R_scratch0, R_scratch0                          ;[1] mask wordPtr to just lower 8 bits

; Test whether this is an extended length wordPtr
	CMP.B   #0, R_scratch0
	JGE		.end

; WordPtr is spread over two bytes -- hopefully we have enough time to compute this (not parsing the WordCount field below helps)
; TODO: Check timings.
.multi_byte:

; Extract lower byte of wordPtr
	MOV.B 	3(R_cmd_ptr), R_scratch1
	MOV.B 	2(R_cmd_ptr), R_scratch3

; This is the same as above -- shift the high bit from CMD[3] into the lower bits of CMD[2]
; Note: the top bit is the next EBV indicator bit, however we only support two byte EBVs, so we ignore it.
	RLC.B	R_scratch1
	RLC.B	R_scratch3
	RLC.B	R_scratch1
	RLC.B	R_scratch3

; Merge upper and lower bytes ((upper & 0x7F) << 7) | (lower & 0x7F))
	AND.B   #0x7F, R_scratch0
	rpt #7	{ rlax.w  R_scratch0
	AND.B   #0x7F, R_scratch3
	BIS.W	R_scratch3, R_scratch0

; Correct CMD offset and bit offset
	ADD 	#8, R_wait
	MOV		#cmd+1, R_cmd_ptr

.end
	MOV.W	R_scratch0, &(RWData.wordPtr)                   ;[3] store the wordPtr


; Impinj hasn't implemented BlockWrite corrctly... it's more of a BulkWrite using BlockWrite rather than a real BlockWrite.
; Instead of using the WordCount to send multiple words, it uses an offset using the WordPtr.

;; Wait until we have all bits to extract WordCount.
;waitOnBits_2:
;	CMP.W   #32, R_bits                                     ;[2] Wait until first 4 bytes are fully received.
;	JLO     waitOnBits_2                                    ;[2]
;
;calc_wordCnt:
;	MOV.B   (cmd+2), R_scratch0                             ;[3] bring in top 6 bits into b5-b0 of R_scratch0 (wordCt.b7-b2)
;	MOV.B   (cmd+3), R_scratch1                             ;[3] bring in bot 2 bits into b7b6  of R_scratch1 (wordCt.b1-b0)
;	RLC.B   R_scratch1                                      ;[1] pull out b7 from R_scratch1 (wordCt.b1)
;	RLC.B   R_scratch0                                      ;[1] shove it into R_scratch0 at bottom (wordCt.b1)
;	RLC.B   R_scratch1                                      ;[1] pull out b7 from R_scratch1 (wordCt.b0)
;	RLC.B   R_scratch0                                      ;[1] shove it into R_scratch0 at bottom (wordCt.b0)
;	MOV.B   R_scratch0, R_scratch0                          ;[1] mask wordPtr to just lower 8 bits
;	MOV.B  R_scratch0, &(RWData.wrData)                     ;[3] store the wordCnt


; Wait for data to write -- Is it worth merging some of this waitOnBit loops together?
waitOnBits_3a:
	ADD 	#16, R_wait
waitOnBits_3:
	CMP.W   R_wait, R_bits                                  ;[2] Wait until first 6 bytes are fully received.
	JLO     waitOnBits_3                                    ;[2]

store_Word:
	MOV.B   3(R_cmd_ptr), R_scratch1                        ;[3] bring in top 6 bits into b5-b0 of R_scratch1 (data.b15-b10)
	MOV.B   4(R_cmd_ptr), R_scratch2                        ;[3] bring in mid 8 bits into b7-b0 of R_scratch2 (data.b9-b2)
	MOV.B   5(R_cmd_ptr), R_scratch0						;[3] bring in bot 2 bits into b7b6  of R_scratch0 (data.b1b0)

	RLC.B   R_scratch2                                      ;[1]
	RLC.B   R_scratch1                                      ;[1]
	RLC.B   R_scratch2                                      ;[1]
	RLC.B   R_scratch1                                      ;[1]
	RRC.B   R_scratch2                                      ;[1]
	RRC.B   R_scratch2                                      ;[1]

	RLC.B   R_scratch0                                      ;[1]
	RLC.B   R_scratch2                                      ;[1]
	RLC.B   R_scratch0                                      ;[1]
	RLC.B   R_scratch2                                      ;[1]

	SWPB    R_scratch1                                      ;[1]
	BIS     R_scratch2, R_scratch1                          ;[1] merge b15-b8(R_scratch1) and b7-b(R_scratch2) together into R_scratch1


; Write received word to either the currently configured BlockWrite buffer for regular messages
; or to the control message field for control messages.
;
; This is done to allow us to use the wordPtr for extra data in the case of control messages.
	MOV.B   &(RWData.memBank), R_scratch0					;[] Retrieve MEM Bank
	TST.B   R_scratch0										;[] Check if this is a control command
	JNZ		store_regular
store_control:
	MOV     R_scratch1, &(RWData.controlMessage)
	JMP 	waitOnBits_4a
store_regular:
	MOV.W   &(RWData.wordPtr), R_scratch2                   ;[3] Put offset to R15
	RLAM.A  #1, R_scratch2                                  ;[2] Offset *= 2
	ADDX.A  &(RWData.bwrBufPtr), R_scratch2                 ;[3] Add base address to offset.
	MOV     R_scratch1, 0(R_scratch2)                       ;[3] move the data out to the correct address.

; Wait on handle.
waitOnBits_4a:
	ADD 	#16, R_wait
waitOnBits_4:
	CMP.W   R_wait, R_bits                                  ;[2]
	JLO     waitOnBits_4                                    ;[2]

	MOV.B   R_wait, &(RWData.bitCount)

; Pull handle into R_scratch1.
	MOV.B   5(R_cmd_ptr), R_scratch1                        ;[3] bring in top 6 bits into b5-b0 of R_scratch1 (data.b15-b10)
	MOV.B   6(R_cmd_ptr), R_scratch2                        ;[3] bring in mid 8 bits into b7-b0 of R_scratch2 (data.b9-b2)
	MOV.B   7(R_cmd_ptr), R_scratch0                        ;[3] bring in bot 2 bits into b7b6  of R_scratch0 (data.b1b0)

	RLC.B   R_scratch2                                      ;[1]
	RLC.B   R_scratch1                                      ;[1]
	RLC.B   R_scratch2                                      ;[1]
	RLC.B   R_scratch1                                      ;[1]
	RRC.B   R_scratch2                                      ;[1]
	RRC.B   R_scratch2                                      ;[1]

	RLC.B   R_scratch0                                      ;[1]
	RLC.B   R_scratch2                                      ;[1]
	RLC.B   R_scratch0                                      ;[1]
	RLC.B   R_scratch2                                      ;[1]

	SWPB    R_scratch1                                      ;[1]
	BIS     R_scratch2, R_scratch1                          ;[1] merge b15-b8(R_scratch1) and b7-b(R_scratch2) together into R_scratch1

; Check if handle matches.
	CMP     R_scratch1, &rfid.handle                        ;[2]
	JNE     exit_safely                                     ;[2] Handle doesn't match, so exit.

; Prepare rfid transmission buffer, CRC16 0-bit and handle.
	MOV     (rfid.handle), R_scratch0                       ;[3] bring in the RN16
	SWPB    R_scratch0                                      ;[1] swap bytes so we can shove full word out in one call (MSByte into dataBuf[0],...)
	MOV     R_scratch0, &(rfidBuf)                          ;[3] load the MSByte

	MOV     #(rfidBuf), R_scratch2                          ;[2] load &dataBuf[0] as dataPtr
	MOV     #(2), R_scratch1                                ;[2] load num of bytes in ACK

	MOV     #ZERO_BIT_CRC, R12                              ;[2]
	CALLA   #crc16_ccitt                                    ;[5+196]

	MOV.B   R12, &(rfidBuf+3)                               ;[3] store lower CRC byte first
	SWPB    R12                                             ;[1] move upper byte into lower byte
	MOV.B   R12, &(rfidBuf+2)                               ;[3] store upper CRC byte

	CLRC                                                    ;[3]
	RRC.B   (rfidBuf)                                       ;[6]
	RRC.B   (rfidBuf+1)                                     ;[6]
	RRC.B   (rfidBuf+2)                                     ;[6]
	RRC.B   (rfidBuf+3)                                     ;[6]
	RRC.B   (rfidBuf+4)                                     ;[6]

; Wait for the rest of the BlockWrite command bits (CRC16).
	MOV.B   &(RWData.bitCount), R_wait
	ADD 	#10, R_wait
waitOnBits_5:
	CMP.W   R_wait, R_bits                                  ;[2]
	JLO     waitOnBits_5                                    ;[2]

; TODO: Figure out when we REALLY need to respond to the reader... commercial tags are not responding before they have written ALL words.

; Reset cmd buffer.
	MOV     #(cmd), R4                                      ;[] Now reset cmd buffer.
	MOV     #(-3), R_bits                                   ;[] Prepare to parse frame sync.
	CLR     R6                                              ;[] Clear the bitcount.

; We are done... now disable interrupt and wait at least RTCAL*0.85 - 2us before sending (Table 6.16 EPC C1G2)
	DINT                                                    ;[3]
	NOP                                                     ;[1]
	CLR     &TA0CTL                                         ;[4]

; TCAL*0.85 - 2 us <= DELAY before response <= 20 ms
;call_my_BlockWriteCallback:
;	CMP.B       #(0), &(RWData.bwrHook)                     ;[4]
;	JEQ         move_timing_delay_BlockWrite                ;[2] If there is no user callback, wait before responding.
;	MOV         &(RWData.bwrHook), R_scratch0               ;[3]
;	CALLA       R_scratch0                                  ;[5]


; Check if we need to decrypt this block
;	MOV.B   	&(RWData.memBank), R_scratch0					;[] Retrieve MEM Bank
;	TST.B   	R_scratch0										;[] Check if this is a control command
;	JZ			move_timing_delay_BlockWrite

;	MOV.W   	&(RWData.wordPtr), R_scratch0
;	AND.B		#7, R_scratch0
;	CMP.W 		#7, R_scratch0
;	JEQ 		decryptBlock

; Just in case the user has no BlockWrite callback, we delay the response.
move_timing_delay_BlockWrite:
	MOV     #750, R_scratch0                                ;[2] Why is this value working for both readers? WTF happens during T5 anyway!?

timing_delay_for_BlockWrite:
	DEC     R_scratch0                                      ;[1]
	JNZ     timing_delay_for_BlockWrite                     ;[2]

respond_to_BlockWrite:
	MOV     #rfidBuf, R12                                   ;[2] load the &rfidBuf[0]
	MOV     #(4), R_scratch2                                ;[1] load into corr reg (numBytes)
	MOV     #1, R_scratch1                                  ;[1] load numBits=1
	MOV.B   #TREXT_ON, R_scratch0                           ;[3] load TRext

	CALLA   #TxFM0                                          ;[5] Send response.

; TODO: In what order do we receive the words!? Figure out correct stop condition.
; Experimental, breaks BlockWrite atm... pls fix.
;blockwriteHandle_SkipHookCall:
;	BIT.B	#(CMD_ID_BLOCKWRITE), (rfid.abortOn)            ;[] Should we abort on BlockWrite?
;	JNZ		blockwriteHandle_BreakOutofRFID                 ;[]
;	RETA                                                    ;[] else return w/o setting flag


;blockwriteHandle_BreakOutofRFID:
;	BIS.B	#1, (rfid.abortFlag)                            ;[] by setting this bit we'll abort correctly!
;	CALLA   #RxClock                                        ;[5+x] Switch to RxClock
;	BIC     #(GIE), SR                                      ;[1] don't need anymore bits, so turn off Rx_SM
;	NOP
;	CLR     &TA0CTL
;	RETA

update_authentication_state:
	MOV.B   &(RWData.memBank), R_scratch0					;[] Retrieve MEM Bank
	TST.B   R_scratch0										;[] Check if this is a control command
	JNZ		update_rx_message

update_control_message:
	DINT
	NOP;
	CALLA 	#blockWriteControlMessage
	JMP 	exit_safely

update_rx_message:
	MOV.W   	&(RWData.wordPtr), R_scratch0
	BIT.W 		#7, R_scratch0
	JNE   		.skip_decrypt
	CALLA   	#decryptCurrentBlock
.skip_decrypt:
	CALLA 	#blockWriteRxWord

exit_safely:
	DINT
	NOP;
	CLR &TA0CTL;
	RETA;

	.end
