#ifndef _FILTER_H
#define _FILTER_H


// Fixed-Point Exponential Moving Average
// alpha = 1/2^k
// x: output value
// s: current sample
#define U16_FP_EMA_K2(x, s)  (u16)((((u32)(x)<<2) - (x) + (s)) >> 2)
#define U16_FP_EMA_K4(x, s)  (u16)((((u32)(x)<<4) - (x) + (s)) >> 4)
#define U16_FP_EMA_K8(x, s)  (u16)((((u32)(x)<<8) - (x) + (s)) >> 8)
#define U16_FP_EMA_K16(x, s) (u16)((((u32)(x)<<16) - (x) + (s)) >> 16)

#define I16_FP_EMA_K2(x, s)  (s16)((((s32)(x)<<2) - (x) + (s)) >> 2)
#define I16_FP_EMA_K4(x, s)  (s16)((((s32)(x)<<4) - (x) + (s)) >> 4)
#define I16_FP_EMA_K8(x, s)  (s16)((((s32)(x)<<8) - (x) + (s)) >> 8)
#define I16_FP_EMA_K16(x, s) (s16)((((s32)(x)<<16) - (x) + (s)) >> 16)


#endif // _FILTER_H
