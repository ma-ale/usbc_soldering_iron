#include <u8g2.h>
#include <ch32fun.h>
#include <stdio.h>

#include "display.h"
#include "lib_i2c.h"


static u8g2_t u8g2;
static const char digits_lut[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";


// GPIO and delay callback for u8g2/u8x8 display driver
static uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	switch(msg) {
	case U8X8_MSG_GPIO_AND_DELAY_INIT:
		// called once during init phase of u8g2/u8x8
		// can be used to setup pins
		printf("TODO: U8X8_MSG_GPIO_AND_DELAY_INIT\n");
		break;
	case U8X8_MSG_DELAY_NANO:
		// delay arg_int * 1 nano second
		// at 48MHz 1 cycle = 20ns, 1 nop = 1 cycle
		// the for lopp takes about 2 cycles per iteration
		for (int i = 0; i < (arg_int/(20*3)); i++) {
			asm("nop");
		}
		break;
	case U8X8_MSG_DELAY_100NANO:
		// delay arg_int * 100 nano seconds
		// at 48MHz 1 cycle = 20ns, 1 nop = 1 cycle
		// the for lopp takes about 2 cycles per iteration
		for (int i = 0; i < ((arg_int*100)/(20*3)); i++) {
			asm("nop");
		}
		break;
	case U8X8_MSG_DELAY_10MICRO:
		// delay arg_int * 10 micro seconds
		Delay_Us(arg_int*10);
		break;
	case U8X8_MSG_DELAY_MILLI:
		// delay arg_int * 1 milli second
		Delay_Ms(arg_int);
		break;
	case U8X8_MSG_DELAY_I2C:
		// arg_int is the I2C speed in 100KHz, e.g. 4 = 400 KHz
		// arg_int=1: delay by 5us, arg_int = 4: delay by 1.25us
		printf("TODO: U8X8_MSG_DELAY_I2C\n");
		break;
	case U8X8_MSG_GPIO_D0:
		// D0 or SPI clock pin: Output level in arg_int
	//case U8X8_MSG_GPIO_SPI_CLOCK:
		break;
	case U8X8_MSG_GPIO_D1:
		// D1 or SPI data pin: Output level in arg_int
	//case U8X8_MSG_GPIO_SPI_DATA:
		break;
	case U8X8_MSG_GPIO_D2:
		// D2 pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_D3:
		// D3 pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_D4:
		// D4 pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_D5:
		// D5 pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_D6:
		// D6 pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_D7:
		// D7 pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_E:
		// E/WR pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_CS:
		// CS (chip select) pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_DC:
		// DC (data/cmd, A0, register select) pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_RESET:
		funDigitalWrite(PIN_DISP_RST, arg_int);
		// Reset pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_CS1:
		// CS1 (chip select) pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_CS2:
		// CS2 (chip select) pin: Output level in arg_int
		break;
	case U8X8_MSG_GPIO_I2C_CLOCK:
		// arg_int=0: Output low at I2C clock pin
		// arg_int=1: Input dir with pullup high for I2C clock pin
		break;
	case U8X8_MSG_GPIO_I2C_DATA:
		// arg_int=0: Output low at I2C data pin
		// arg_int=1: Input dir with pullup high for I2C data pin
		break;
	case U8X8_MSG_GPIO_MENU_SELECT:
		// get menu select pin state
		u8x8_SetGPIOResult(u8x8, 0);
		break;
	case U8X8_MSG_GPIO_MENU_NEXT:
		// get menu next pin state
		u8x8_SetGPIOResult(u8x8, 0);
		break;
	case U8X8_MSG_GPIO_MENU_PREV:
		// get menu prev pin state
		u8x8_SetGPIOResult(u8x8, 0);
		break;
	case U8X8_MSG_GPIO_MENU_HOME:
		// get menu home pin state
		u8x8_SetGPIOResult(u8x8, 0);
		break;
	default:
		// default return value
		u8x8_SetGPIOResult(u8x8, 1);
		break;
	}
	return 1;
}


static uint8_t u8x8_byte_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	static uint8_t buffer[32];
	static uint8_t buffer_idx;
	uint8_t *data;

	switch(msg) {
	case U8X8_MSG_BYTE_INIT:
		break;
	case U8X8_MSG_BYTE_SEND:
		data = (uint8_t*)arg_ptr;
		while (arg_int > 0) {
			buffer[buffer_idx++] = *data;
			data++;
			arg_int--;
		}
		break;
	case U8X8_MSG_BYTE_START_TRANSFER:
		buffer_idx = 0;
		break;
	case U8X8_MSG_BYTE_END_TRANSFER:
		i2c_sendBytes(I2C_TARGET, u8x8_GetI2CAddress(u8x8) >> 1, buffer, buffer_idx);
		break;
	default:
		return 0;
	}
	return 1;
}


u8g2_t* display_init(void)
{
#if defined(SSD1306_128X32) && SSD1306_128X32
	u8g2_Setup_ssd1306_i2c_128x32_univision_f(&u8g2, U8G2_R0, u8x8_byte_i2c, u8x8_gpio_and_delay);
#elif defined(SSD1312_96X16) && SSD1312_96X16
	// NOTE: display size is wrong, hardware is 96x16, but driver is configured for 120x28
	u8g2_Setup_ssd1312_i2c_128x32_f(&u8g2, U8G2_R0, u8x8_byte_i2c, u8x8_gpio_and_delay);
#else
	static_assert(0, "unsupported display size");
#endif
	// TODO: log errors and return NULL on failure
	u8g2_InitDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 0);
	u8g2_SetContrast(&u8g2, 255);

	u8g2_SetBitmapMode(&u8g2, 1);
	u8g2_SetFontMode(&u8g2, 1);

	return &u8g2;
}


const char* u16toa(uint16_t value)
{
	// Max uint16_t is 65535 (5 digits) + null terminator
	static char buf[6];
	char *p = &buf[5];
	*p = '\0';

	// Process two digits at a time using the LUT
	while (value >= 100) {
		const unsigned int idx = (value % 100) * 2;
		value /= 100;
		*--p = digits_lut[idx + 1];
		*--p = digits_lut[idx];
	}

	// Handle the remaining value (< 100)
	if (value < 10) {
		*--p = (char)('0' + value);
	} else {
		const unsigned int idx = value * 2;
		*--p = digits_lut[idx + 1];
		*--p = digits_lut[idx];
	}

	return p;
}


const char* i16toa(int16_t value)
{
	static char buf[7];
	uint16_t uval;
	bool negative = false;

	if (value < 0) {
		negative = true;
		uval = (uint16_t)-value;
	} else {
		uval = (uint16_t)value;
	}

	char *p = &buf[6];
	*p = '\0';

	// Same LUT logic as unsigned
	while (uval >= 100) {
		const unsigned int idx = (uval % 100) * 2;
		uval /= 100;
		*--p = digits_lut[idx + 1];
		*--p = digits_lut[idx];
	}

	if (uval < 10) {
		*--p = (char)('0' + uval);
	} else {
		const unsigned int idx = uval * 2;
		*--p = digits_lut[idx + 1];
		*--p = digits_lut[idx];
	}

	if (negative) {
		*--p = '-';
	}

	return p;
}
