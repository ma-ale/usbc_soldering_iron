#include <ch32fun.h>
#include <stdint.h>
#include <stdio.h>
#include <fsusb.h>

// Pin definitions
#define PIN_LED     PA4  // debug led
#define PIN_VBUS    PA0  // vbus voltage feedback
#define PIN_CURRENT PA1  // current feedback
#define PIN_NTC     PA2  // ntc temperature sensor
#define PIN_TEMP    PA3  // thermocouple amplifier
#define PIN_12V     PA5  // 12V regulator enable
#define PIN_HEATER  PA6  // power mosfet gate control
#define PIN_ENC_A   PB3  // rotary encoder A
#define PIN_ENC_B   PB11 // rotary encoder B
#define PIN_BTN     PB1  // rotary encoder button

// Analog channel definitions
#define VBUS_ADC_CHANNEL    ANALOG_0 // PA0
#define CURRENT_ADC_CHANNEL ANALOG_1 // PA1
#define NTC_ADC_CHANNEL     ANALOG_2 // PA2
#define TEMP_ADC_CHANNEL    ANALOG_3 // PA3

// constants
// LUT for converting NTC readings to degrees kelvin
// Nominal: 1kOhm, Beta: 3380, Step: 64
// TODO: Since the board temperature is almost always in the 300-200K range add
// more steps in order to better represent that interval
const uint8_t ntc_step_size = 64;
const int16_t ntc_table[64] = {
    1180, 197, 155, 133, 119, 108, 100, 93, 87, 82, 77, 73, 69, 66, 63, 60, 57, 54, 52, 50, 47, 45, 43, 41, 39, 37, 35, 34, 32, 30, 28, 27, 25, 23, 22, 20, 19, 17, 15, 14, 12, 11, 9, 7, 6, 4, 2, 0, -1, -3, -5, -7, -9, -11, -14, -16, -19, -22, -25, -28, -33, -38, -44, -55
};


uint8_t pin = 0;
int16_t encoder = 0; // rotary encoder counter
uint32_t last_interrupt = 0; // last time the encoder interrupt was triggered
#define ENCODER_DEBOUNCE 6000


// Convert the raw adc reading to a temperature in kelvin with the ntc lut,
// linearly interpolating between positions
static inline int16_t get_temp_k(uint16_t adc_reading)
{
	if (adc_reading > 4095) return 0;
	uint8_t index = adc_reading / ntc_step_size;
	uint8_t remainder = adc_reading % ntc_step_size;
	int16_t temp_base = index < 64 ? ntc_table[index] : 0;
    int16_t temp_next = index < 63 ? ntc_table[index + 1] : temp_base;
    return temp_base + ((temp_next - temp_base) * remainder)/ntc_step_size;
}


// convert the raw tip reading from the adc to a temperature in kelvin
static inline int16_t get_tip_temp_k(uint16_t adc_reading, int16_t cold_temp_k)
{
	return 0;
}


// this callback is mandatory when FUNCONF_USE_USBPRINTF is defined,
// can be empty though
void handle_usbfs_input(int numbytes, uint8_t *data)
{
	// handle single character commands
	// TODO:
	//     - 'c' to calibrate the tip temperature
	//     - 't' to test tip presence
	if(numbytes == 1) {
		switch(data[0]) {
		case 'r': // toggle the 12V regulator
			if (funDigitalRead(PIN_12V)) {
				funDigitalWrite(PIN_12V, 0);
				printf("Disabled 12V Regulator\n");
			} else {
				funDigitalWrite(PIN_12V, 1);
				printf("Enabled 12V Regulator\n");
			}
			break;
		case 'h':
			printf(
				"Available commands:\n"
				"\tr : toggle the 12V regulator\n"
			);
			break;
		default:
			printf("Unknown command '%c'\n", data[0]);
			break;
		}
	}
	else {
		// echo
		// _write(0, (const char*)data, numbytes);
	}
}


// triggered on the falling edge of the rotary encoder PIN_A
void EXTI15_8_IRQHandler(void) __attribute__((interrupt));
void EXTI15_8_IRQHandler(void)
{
	uint32_t now = funSysTick32();
	if (now - last_interrupt > ENCODER_DEBOUNCE) {
		last_interrupt = now;
		if (funDigitalRead(PIN_ENC_A)) {
			encoder++;
		} else {
			encoder--;
		}
	}
	EXTI->INTFR = EXTI_Line11;
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
	funPinMode(PIN_TEMP, GPIO_CFGLR_IN_ANALOG);

	funPinMode(PIN_LED, GPIO_CFGLR_OUT_10Mhz_PP);
	funPinMode(PIN_12V, GPIO_CFGLR_OUT_10Mhz_PP);
	funDigitalWrite(PIN_12V, 0);
	funPinMode(PIN_HEATER, GPIO_CFGLR_OUT_10Mhz_PP);
	funDigitalWrite(PIN_HEATER, 0);

	funPinMode(PIN_ENC_A, GPIO_CFGLR_IN_PUPD); // enable pull-up/down
	funDigitalWrite(PIN_ENC_A, 1); // specify pull-up
	funPinMode(PIN_ENC_B, GPIO_CFGLR_IN_PUPD); // enable pull-up/down
	funDigitalWrite(PIN_ENC_B, 1); // specify pull-up
	funPinMode(PIN_BTN, GPIO_CFGLR_IN_FLOAT);

 	// Configure the IO as an interrupt.
 	// PIN_ENC_B is on port B, channel 11
    AFIO->EXTICR1 = AFIO_EXTICR1_EXTI11_PB; // Port B channel (pin) 11
    EXTI->INTENR = EXTI_INTENR_MR11; // Enable EXT11
    EXTI->FTENR = EXTI_FTENR_TR11;  // Falling edge trigger
    // enable interrupt
    NVIC_EnableIRQ(EXTI15_8_IRQn);

	Delay_Ms(500);

	unsigned int count = 0;

	for (uint32_t x = 0; true ; x++) {
		poll_input(); // usb
		if ((x % 100) == 0) {
			uint16_t vbus_mv = (funAnalogRead(VBUS_ADC_CHANNEL)*3300*11)/4095;
			uint16_t current_ma = ((uint32_t)funAnalogRead(CURRENT_ADC_CHANNEL) * 4125 + 1024) / 2048;
			int16_t temp_k = get_temp_k(funAnalogRead(NTC_ADC_CHANNEL));
			uint16_t temp_tip_k = funAnalogRead(TEMP_ADC_CHANNEL);

			printf("[%d]: VBUS=%d, CURRENT=%d, TEMP=%d, TIP=%d, COUNTER=%d\n", count++, vbus_mv, current_ma, temp_k, temp_tip_k, encoder);

			funDigitalWrite(PIN_LED, pin);
			pin = !pin;
		}
		Delay_Ms(1);
	}
}
