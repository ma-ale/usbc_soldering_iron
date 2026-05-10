#ifndef _FILTER_H
#define _FILTER_H

#include <stdint.h>


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static_assert(-4 >> 1 == -2, ">> doesn't do sign extension");

// Fixed-Point Exponential Moving Average
// alpha = 1/2^k
// x: output value
// s: current sample
#define U16_FP_EMA_K2(x, s)  (uint16_t)((((uint32_t)(x)<<2) - (x) + (s)) >> 2)
#define U16_FP_EMA_K4(x, s)  (uint16_t)((((uint32_t)(x)<<4) - (x) + (s)) >> 4)
#define U16_FP_EMA_K8(x, s)  (uint16_t)((((uint32_t)(x)<<8) - (x) + (s)) >> 8)
#define U16_FP_EMA_K16(x, s) (uint16_t)((((uint32_t)(x)<<16) - (x) + (s)) >> 16)

#define I16_FP_EMA_K2(x, s)  (int16_t)((((int32_t)(x)<<2) - (x) + (s)) >> 2)
#define I16_FP_EMA_K4(x, s)  (int16_t)((((int32_t)(x)<<4) - (x) + (s)) >> 4)
#define I16_FP_EMA_K8(x, s)  (int16_t)((((int32_t)(x)<<8) - (x) + (s)) >> 8)
#define I16_FP_EMA_K16(x, s) (int16_t)((((int32_t)(x)<<16) - (x) + (s)) >> 16)

#endif // _FILTER_H
