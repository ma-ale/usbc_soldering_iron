#ifndef _LINCAL_H
#define _LINCAL_H

#include <ch32fun.h>


// Fixed-point number in 8.8 format
typedef int16_t fp16_t;
#define F32_TO_FP16(f) ((fp16_t)((f) * 256.0f))

// Intger part of fp16_t
#define I(f) ((f) >> 8)
// Decimal part of fp16_t
#define D(f) ((f) & 0xFF)

// v * a + b for signed 16-bit
#define I16_LINCAL(v, a, b) ((int16_t)((((int32_t)(v) * (a)) >> 8) + (b)))
// v * a + b for unsigned 16-bit
#define U16_LINCAL(v, a, b) ((uint16_t)((((uint32_t)(v) * (a)) >> 8) + (b)))


#endif
