#ifndef _FILTER_H
#define _FILTER_H

#include <stdint.h>
#include <assert.h>


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

// Integer square root (binary search)
// https://en.wikipedia.org/wiki/Integer_square_root
static inline uint16_t isqrt(uint32_t x)
{
	uint16_t l = 0;     // lower bound of the square root
	uint16_t r = x + 1; // upper bound of the square root

	while (l != r - 1) {
		uint32_t m = (l + r) / 2; // midpoint to test
		if (m * m <= x) {
			l = m;
		} else {
			r = m;
		}
	}

	return l;
}


#endif // _FILTER_H
