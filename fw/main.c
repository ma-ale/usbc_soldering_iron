#include <ch32fun.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fsusb.h>

#define PIN_LED PA4
uint8_t pin = 0;

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
	funPinMode(PIN_LED, GPIO_CFGLR_OUT_10Mhz_PP);
	USBFSSetup();
	Delay_Ms(500);

	unsigned int count = 0;

	for (uint32_t x = 0; true ; x++) {
		poll_input(); // usb
		if ((x % 100) == 0) {
			printf("[%d]: Hello From CH32X035\n", count++);
			funDigitalWrite(PIN_LED, pin);
			pin = !pin;
		}
		Delay_Ms(1);
	}
}
