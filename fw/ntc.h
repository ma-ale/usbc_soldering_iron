#ifndef _NTC_H
#define _NTC_H

#include <stdint.h>

// LUT for converting NTC readings to degrees celsius
// Nominal: 10kOhm, Beta: 3380, Step: 64
// TODO: Add a fixed offset to account for toollerances
const uint8_t ntc_step_size = 64;
const int16_t ntc_lut[] = {
	1316, 197, 155, 133, 119, 108, 100, 93, 87, 82, 77, 73, 69, 66, 63, 60,
	57, 54, 52, 50, 47, 45, 43, 41, 39, 37, 35, 34, 32, 30, 28, 27,
	25, 23, 22, 20, 19, 17, 15, 14, 12, 11, 9, 7, 6, 4, 2, 0,
	-1, -3, -5, -7, -9, -11, -14, -16, -19, -22, -25, -28, -32, -38, -44, -55,
	-55 // extra value to not have an extra if statement
};

// Convert the raw adc reading to a temperature in celsius with the ntc lut,
// linearly interpolating between positions
static inline int16_t get_temp_c(uint16_t adc_reading)
{
	if (adc_reading > 4095) return 0;
	uint8_t index = adc_reading / ntc_step_size;
	uint8_t remainder = adc_reading % ntc_step_size;
	int16_t temp_base = index < ntc_step_size ? ntc_lut[index] : 0;
	int16_t temp_next = ntc_lut[index + 1];
	return temp_base + ((temp_next - temp_base) * remainder)/ntc_step_size;
}


#endif
