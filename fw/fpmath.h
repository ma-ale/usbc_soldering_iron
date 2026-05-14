#ifndef _FPMATH_H
#define _FPMATH_H

#include <stdint.h>
#include <assert.h>

// This library depends on sign extension
static_assert(-4 >> 1 == -2, ">> doesn't do sign extension");


// Fixed point signed type in 16.16 format
#define FP_R 16
typedef int32_t fp16_t;


#define POW10(d) (                                                     \
    ((d) == 0) ? 1L :                                                  \
    ((d) == 1) ? 10L :                                                 \
    ((d) == 2) ? 100L :                                                \
    ((d) == 3) ? 1000L :                                               \
    ((d) == 4) ? 10000L :                                              \
    ((d) == 5) ? 100000L :                                             \
    ((d) == 6) ? 1000000L :                                            \
    ((d) == 7) ? 10000000L :                                           \
    ((d) == 8) ? 100000000L : 1000000000L)


static inline fp16_t i2fp(int16_t i)
{
	return (int32_t)i << FP_R;
}


static inline int16_t fp2i(fp16_t f)
{
	return (int16_t)(f >> FP_R);
}


static inline fp16_t num2fp(int16_t i, int16_t f, uint8_t d)
{
	return i2fp(i) + (i < 0 ? -(i2fp(f)/POW10(d)) : i2fp(f)/POW10(d));
}


// Saturating addition
static inline fp16_t fp_add(fp16_t a, fp16_t b)
{
	fp16_t r;
	if (__builtin_add_overflow(a, b, &r)) {
		// Positive overflow
		if (a > 0 && b > 0) {
			r = INT32_MAX;
		} else {
			r = INT32_MIN;
		}
	}
	return r;
}


static inline fp16_t fp_sub(fp16_t a, fp16_t b)
{
	fp16_t r;
	if (__builtin_sub_overflow(a, b, &r)) {
		if (a >= 0 && b < 0) {
			r = INT32_MAX;
		} else {
			r = INT32_MIN;
		}
	}
	return r;
}


// Saturating multiplication
static inline fp16_t fp_mul(fp16_t a, fp16_t b)
{
	int64_t r = ((int64_t)a * (int64_t)b + 0x8000) >> 16;
	if (r >  (int64_t)0x7fffffff) return INT32_MAX;
	if (r < -(int64_t)0x80000000) return INT32_MIN;
	return r;
}


static inline fp16_t fp_div(fp16_t a, fp16_t b)
{
	if (b == 0) {
		return (a >= 0) ? INT32_MAX : INT32_MIN;
	}

	int64_t r = ((int64_t)a << 16) / b;

	// Saturate to 32-bit bounds
	if (r > (int64_t)0x7fffffff) return INT32_MAX;
	if (r < -(int64_t)0x80000000) return INT32_MIN;

	return (fp16_t)r;
}


static inline fp16_t fp_clamp(fp16_t x, fp16_t min, fp16_t max)
{
	return x < min ? min : (x > max ? max : x);
}


static inline fp16_t fp_map(fp16_t x, fp16_t src_start, fp16_t src_end, fp16_t dst_start, fp16_t dst_end)
{
	fp16_t x_offset = fp_sub(x, src_start);
	fp16_t dst_range = fp_sub(dst_end, dst_start);
	fp16_t src_range = fp_sub(src_end, src_start);
	fp16_t scaled = fp_mul(x_offset, dst_range);
	fp16_t mapped_offset = fp_div(scaled, src_range);
	return fp_add(mapped_offset, dst_start);
}


#endif // _FPMATH_H
