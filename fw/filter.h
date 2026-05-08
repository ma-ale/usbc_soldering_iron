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

/* ------------------------- FIXED POINT OPERATIONS ------------------------- */

// Fixed-point number in 8.8 format
typedef int16_t fp16_t;
#define F32_TO_FP16(f) ((fp16_t)((f) * 256.0f))
#define I16_TO_FP16(i) ((fp16_t)((i) << 8))
#define U16_TO_FP16(u) ((fp16_t)((u) << 8))

// Intger part of fp16_t
#define I(f) ((f) >> 8)
// Decimal part of fp16_t
#define D(f) ((f) & 0xFF)

static inline int16_t i16_mul_fp16(int16_t v, fp16_t f)
{
	return (((int32_t)(v)<<8) * f) >> 8;
}

static inline uint16_t u16_mul_fp16(uint16_t v, fp16_t f)
{
	return (((uint32_t)(v)<<8) * f) >> 8;
}

// v * a + b for signed 16-bit
#define I16_LINCAL(v, a, b) ((int16_t)((((int32_t)(v)<<8) * (a) + (b)) >> 8))
// v * a + b for unsigned 16-bit
#define U16_LINCAL(v, a, b) ((uint16_t)((((uint32_t)(v)<<8) * (a) + (b)) >> 8))


// Fixed-point number in 24.8 format
typedef uint32_t fp24_8_t;

static inline fp24_8_t f32_to_fp24_8(float f)
{
	return ((fp24_8_t)((f) * 256.0f));
}

static inline fp24_8_t i16_to_fp24_8(int16_t i)
{
	return ((fp24_8_t)(i)) << 8;
}

static inline fp24_8_t u16_to_fp24_8(uint16_t u)
{
	return ((fp24_8_t)(u)) << 8;
}

static inline fp24_8_t fp24_8_mul(fp24_8_t a, fp24_8_t b)
{
	return ((fp24_8_t)((uint32_t)a * b) >> 8);
}


#endif // _FILTER_H
