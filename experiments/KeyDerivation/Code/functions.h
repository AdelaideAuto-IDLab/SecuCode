/*
 * functions.h
 *
 *  Created on: Aug 31, 2017
 *      Author: User
 */

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_
#include "wisp-base.h"

//================================================================sleep related
//1 number = 0.1 mS
#define SLEEPTIME 300
#pragma vector=TIMER3_A0_VECTOR
__interrupt void TIMER3_A0_ISR(void)
{
    __bic_SR_register_on_exit(LPM3_bits|GIE);
}
inline void sleeplpm3(){
    if(SLEEPTIME>0){
        // Use Timer 3A for a short delay to accumulate power===============
        TA3CCTL0 = CCIE;                        // TACCR0 interrupt enabled
        TA3CCR0 = SLEEPTIME;
        TA3CTL = TASSEL__ACLK | MC__UP;         // SMCLK, UP mode
        __bis_SR_register(LPM3_bits | GIE);
        TA3CTL &= ~(MC__UP);                    // Stop Timer
        //==================================================================
    }
}

//================================================================PUF related
inline void encode_bch(unsigned char *ri, unsigned char *helper,unsigned char *key){
    unsigned int    idx, j;
    signed int i;// i need to use at 0
    unsigned char    feedback;

    unsigned char bb[15];
    const int length = 31;//===========fixed parameter
    const int k = 16;//key length
    const char g[16] = {1,1,1,1,0,1,0,1,1,1,1,1,0,0,0,1};

    //parallel to serial engine
    unsigned char data[31];//put each bit into an individual byte 10100101 =>00000001, 00000000, 00000001, 00000000, 00000000, 00000001, 00000000, 00000001
    for(idx = 0;idx < length; idx++){
        data[idx] = (ri[idx/8] & (0x1 << (7-(idx%8)))) != 0;
    }

    //generate key for SKE decoder.
    //use the first K bits as the Key "Cryptographic Key Generation from PUF Data Using Efficient Fuzzy Extractor"
    key[0] = ri[0];
    key[1] = ri[1];

    for (i = 0; i < length - k; i++)
        bb[i] = 0;
    for (i = k - 1; i >= 0; i--) {
        feedback = data[i] ^ bb[length - k - 1];
        if (feedback != 0) {
            for (j = length - k - 1; j > 0; j--)
                if (g[j] != 0)
                    bb[j] = bb[j - 1] ^ feedback;
                else
                    bb[j] = bb[j - 1];
            bb[0] = g[0] && feedback;
        } else {
            for (j = length - k - 1; j > 0; j--)
                bb[j] = bb[j - 1];
            bb[0] = 0;
        }
    }


    //serial to parallel engine
    idx = 0;
    for(i = 0;i < 2;i++){
        helper[i] = 0;
        for(j = 0;j < 8;j++){
            if(idx >= 15){
                helper[i] &= ~0x1;
                break;
            }
            helper[i] |= (bb[idx] << (7-j));
            idx++;
        }
    }

    helper[0] ^= ri[2];
    helper[1] ^= ri[3];
    helper[1] &= ~0x1;//make sure the last unused bit is 0;
}

//=======================================================================TRNG related
void SRAMTRNG(uint16_t *nonce){
    uint16_t *challenge_p = (uint16_t *)0x001C00;
    int i,j;
    for(j=0;j<4;j++){
        for( i= 0;i<8;i++){
            *nonce ^= *challenge_p;
            challenge_p++;
        }
        nonce++;
    }
}


#endif /* FUNCTIONS_H_ */
