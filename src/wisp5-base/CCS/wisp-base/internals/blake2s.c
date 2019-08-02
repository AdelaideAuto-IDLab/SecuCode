// blake2s.c
// A simple blake2s Reference Implementation.

#include "blake2s.h"
#include <msp430.h>

inline void sleeplpm3(uint16_t sleepTime) {
    if(sleepTime > 0){
        // Use Timer 3A for a short delay to accumulate power===============
        TA3CCTL0 = CCIE;                        // TACCR0 interrupt enabled
        TA3CCR0 = sleepTime;
        TA3CTL = TASSEL__ACLK | MC__UP;         // SMCLK, UP mode
        __bis_SR_register(LPM3_bits | GIE);
        TA3CTL &= ~(MC__UP);                    // Stop Timer
        //==================================================================
    }
}

// Cyclic right rotation.

#ifndef ROTR32
#define ROTR32(x, y)  (((x) >> (y)) ^ ((x) << (32 - (y))))
#endif

// Little-endian byte access.

#define B2S_GET32(p)                            \
    (((uint32_t) ((uint8_t *) (p))[0]) ^        \
    (((uint32_t) ((uint8_t *) (p))[1]) << 8) ^  \
    (((uint32_t) ((uint8_t *) (p))[2]) << 16) ^ \
    (((uint32_t) ((uint8_t *) (p))[3]) << 24))

// Mixing function G.

#define B2S_G(a, b, c, d, x, y) {   \
    v[a] = v[a] + v[b] + x;         \
    v[d] = ROTR32(v[d] ^ v[a], 16); \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR32(v[b] ^ v[c], 12); \
    v[a] = v[a] + v[b] + y;         \
    v[d] = ROTR32(v[d] ^ v[a], 8);  \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR32(v[b] ^ v[c], 7); }

// Initialization Vector.

static const uint32_t blake2s_iv[8] =
{
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

// Compression function. "last" flag indicates last block.

static void blake2s_compress(blake2s_ctx *ctx, int last)
{
#ifdef NOTHING
    const uint8_t sigma[10][16] = {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
        { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
        { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
        { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
        { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
        { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
        { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
        { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
        { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
        { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 }
    };
    int i;
    uint32_t v[16], m[16];

    for (i = 0; i < 8; i++) {           // init work variables
        v[i] = ctx->h[i];
        v[i + 8] = blake2s_iv[i];
    }

    v[12] ^= ctx->t[0];                 // low 32 bits of offset
    v[13] ^= ctx->t[1];                 // high 32 bits
    if (last)                           // last block flag set ?
        v[14] = ~v[14];

    for (i = 0; i < 16; i++)            // get little-endian words
        m[i] = B2S_GET32(&ctx->b[4 * i]);

    for (i = 0; i < 10; i++) {          // ten rounds
        B2S_G( 0, 4,  8, 12, m[sigma[i][ 0]], m[sigma[i][ 1]]);
        B2S_G( 1, 5,  9, 13, m[sigma[i][ 2]], m[sigma[i][ 3]]);
        B2S_G( 2, 6, 10, 14, m[sigma[i][ 4]], m[sigma[i][ 5]]);
        B2S_G( 3, 7, 11, 15, m[sigma[i][ 6]], m[sigma[i][ 7]]);
        B2S_G( 0, 5, 10, 15, m[sigma[i][ 8]], m[sigma[i][ 9]]);
        B2S_G( 1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);
        B2S_G( 2, 7,  8, 13, m[sigma[i][12]], m[sigma[i][13]]);
        B2S_G( 3, 4,  9, 14, m[sigma[i][14]], m[sigma[i][15]]);
    }

    for( i = 0; i < 8; ++i )
        ctx->h[i] ^= v[i] ^ v[i + 8];
#endif
}

// Initialize the hashing context "ctx" with optional key "key".
//      1 <= outlen <= 32 gives the digest size in bytes.
//      Secret key (also <= 32 bytes) is optional (keylen = 0).

int blake2s_init(blake2s_ctx *ctx, size_t outlen,
    const void *key, size_t keylen,     // (keylen=0: no key)
    uint16_t sleepTime)
{
#ifdef NOTHING
    size_t i;

    if (outlen == 0 || outlen > 32 || keylen > 32)
        return -1;                      // illegal parameters
    sleeplpm3(sleepTime);
    for (i = 0; i < 8; i++)             // state, "param block"
        ctx->h[i] = blake2s_iv[i];
    ctx->h[0] ^= 0x01010000 ^ (keylen << 8) ^ outlen;

    ctx->t[0] = 0;                      // input count low word
    ctx->t[1] = 0;                      // input count high word
    ctx->c = 0;                         // pointer within buffer
    ctx->outlen = outlen;
    for (i = keylen; i < 64; i++)       // zero input block
        ctx->b[i] = 0;
    sleeplpm3(sleepTime);
    if (keylen > 0) {
        blake2s_update(ctx, key, keylen, sleepTime);
        ctx->c = 64;                    // at the end
    }
#endif
    return 0;
}

// Add "inlen" bytes from "in" into the hash.

void blake2s_update(blake2s_ctx *ctx,
    const void *in, size_t inlen,       // data bytes
    uint16_t sleepTime)
{
#ifdef NOTHING
    size_t i;

    for (i = 0; i < inlen; i++) {
        if (ctx->c == 64) {             // buffer full ?
            ctx->t[0] += ctx->c;        // add counters
            if (ctx->t[0] < ctx->c)     // carry overflow ?
                ctx->t[1]++;            // high word
            sleeplpm3(sleepTime);
            blake2s_compress(ctx, 0);   // compress (not last)
            ctx->c = 0;                 // counter to zero
        }
        ctx->b[ctx->c++] = ((const uint8_t *) in)[i];
    }
#endif
}

// Generate the message digest (size given in init).
//      Result placed in "out".

void blake2s_final(blake2s_ctx *ctx, void *out, uint16_t sleepTime)
{
#ifdef NOTHING
    size_t i;

    ctx->t[0] += ctx->c;                // mark last block offset
    if (ctx->t[0] < ctx->c)             // carry overflow
        ctx->t[1]++;                    // high word

    sleeplpm3(sleepTime);
    while (ctx->c < 64)                 // fill up with zeros
        ctx->b[ctx->c++] = 0;
    blake2s_compress(ctx, 1);           // final block flag = 1

    // little endian convert and store
    sleeplpm3(sleepTime);
    for (i = 0; i < ctx->outlen; i++) {
        ((uint8_t *) out)[i] =
            (ctx->h[i >> 2] >> (8 * (i & 3))) & 0xFF;
    }
#endif
}

// Convenience function for all-in-one computation.

int blake2s(void *out, size_t outlen,
    const void *key, size_t keylen,
    const void *in, size_t inlen)
{
    return blake2s_withSleep(out, outlen, key, keylen, in, inlen, 200);
}

int blake2s_withSleep(void *out, size_t outlen,
    const void *key, size_t keylen,
    const void *in, size_t inlen,
    uint16_t sleepTime)
{
    blake2s_ctx ctx;

    if (blake2s_init(&ctx, outlen, key, keylen, sleepTime))
        return -1;
    blake2s_update(&ctx, in, inlen, sleepTime);
    blake2s_final(&ctx, out, sleepTime);

    return 0;
}
