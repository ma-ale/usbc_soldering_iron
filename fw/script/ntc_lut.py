#!/usr/bin/env python3

import math

# Script to generate NTC lookup table
#   ^ Vcc
#   |
# [RES]
#   |
#   +-- Vadc
#   |
# [NTC]
#   |
#  _|_ GND
#  ///

adc_res = 12
ntc_value = 10_000 # at 25°C
resistor_value = 10_000
ntc_beta = 3380 # at 25°C
steps = 64

# NTC resistance to temperature (°C) conversion
def resistance_to_t(ntc_r):
	# Standard B-parameter equation
    # T = 1 / (1/T0 + 1/B * ln(R/R0))
	inv_t = 1/(25 + 273.15) + math.log(ntc_r/ntc_value)/ntc_beta
	return 1/inv_t - 273.15

# Resistor divider percentage to NTC resistance conversion
# x = NTC / (NTC + RES)  => NTC = RES * x / (1 - x)
def x_to_r(x):
	x = max(0.0001, min(0.9999, x))
	return resistor_value * x / (1 - x)

# Use 4096 for cleaner divisions
adc_max = 2**adc_res
step_size = adc_max // steps

values = []
for i in range(steps):
	adc_val = i * step_size
	x = adc_val / adc_max
	r = x_to_r(adc_val / adc_max)
	t = resistance_to_t(r)
	values.append(int(round(t)))

def to_c_array(values, ctype="float", name="table", formatter=str, colcount=8):
    # apply formatting to each element
    values = [formatter(v) for v in values]
    # split into rows with up to `colcount` elements per row
    rows = [values[i:i+colcount] for i in range(0, len(values), colcount)]
    # separate elements with commas, separate rows with newlines
    body = ',\n    '.join([', '.join(r) for r in rows])
    # assemble components into the complete string
    return '{} {}[] = {{\n    {}}};'.format(ctype, name, body)

print("uint8_t ntc_step_size = {};".format(int(step_size)))
print(to_c_array(values, ctype="int16_t", name="ntc_lut", formatter=str, colcount=16))
