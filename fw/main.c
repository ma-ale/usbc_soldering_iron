#include <ch32fun.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fsusb.h>

// Pin definitions
#define PIN_LED     PA4
#define PIN_VBUS    PA0
#define PIN_CURRENT PA1
#define PIN_NTC     PA2

// Analog channel definitions
#define VBUS_ADC_CHANNEL    ANALOG_0
#define CURRENT_ADC_CHANNEL ANALOG_1
#define NTC_ADC_CHANNEL     ANALOG_2

// constants
// LUT for converting NTC readings to degrees kelvin
// Nominal: 1kOhm, Beta: 3380, Step: 64
const uint8_t ntc_step_size = 64;
const int16_t ntc_table[64] = {
    1180, 197, 155, 133, 119, 108, 100, 93, 87, 82, 77, 73, 69, 66, 63, 60, 57, 54, 52, 50, 47, 45, 43, 41, 39, 37, 35, 34, 32, 30, 28, 27, 25, 23, 22, 20, 19, 17, 15, 14, 12, 11, 9, 7, 6, 4, 2, 0, -1, -3, -5, -7, -9, -11, -14, -16, -19, -22, -25, -28, -33, -38, -44, -55
};


uint8_t pin = 0;


int get_temp_k(int adc_reading)
{
	if (adc_reading < 0 || adc_reading > 4095) return 0;
	uint8_t index = adc_reading / ntc_step_size;
	uint8_t remainder = adc_reading % ntc_step_size;
	if (index > 64) return 0;
	int16_t temp_base = ntc_table[index];
    int16_t temp_next = ntc_table[index + 1];
    int16_t temp_diff = temp_next - temp_base;
    int16_t offset = (temp_diff * remainder) / ntc_step_size;
    return temp_base + offset;
}

// this callback is mandatory when FUNCONF_USE_USBPRINTF is defined,
// can be empty though
void handle_usbfs_input(int numbytes, uint8_t *data)
{
	if(numbytes == 1) {
		switch(data[0]) {
		case 'b':
			printf("HAHAHA\n");
			break;
		}
	}
	else {
		_write(0, (const char*)data, numbytes);
	}
}


__attribute__((noreturn)) int main(void)
{
	SystemInit();
	funGpioInitAll();
	funAnalogInit();
	USBFSSetup();

	funPinMode(PIN_VBUS, GPIO_CFGLR_IN_ANALOG);
	funPinMode(PIN_CURRENT, GPIO_CFGLR_IN_ANALOG);
	funPinMode(PIN_NTC, GPIO_CFGLR_IN_ANALOG);
	funPinMode(PIN_LED, GPIO_CFGLR_OUT_10Mhz_PP);

	Delay_Ms(500);

	unsigned int count = 0;

	for (uint32_t x = 0; true ; x++) {
		poll_input(); // usb
		if ((x % 100) == 0) {
			int vbus_mv = (funAnalogRead(VBUS_ADC_CHANNEL)*3300*11)/4095;
			int current_ma = (funAnalogRead(CURRENT_ADC_CHANNEL)*3300)/4095;
			int temp_k = get_temp_k(funAnalogRead(NTC_ADC_CHANNEL));
			printf("[%d]: Hello From CH32X035, VBUS=%d, CURRENT=%d, TEMP=%d\n", count++, vbus_mv, current_ma, temp_k);
			funDigitalWrite(PIN_LED, pin);
			pin = !pin;
		}
		Delay_Ms(1);
	}
}
