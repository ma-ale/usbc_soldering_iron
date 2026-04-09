#include <ch32fun.h>
#include <stdio.h>
#include <string.h>
#include <fsusb.h>


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
	USBFSSetup();

	unsigned int count = 0;
	while (1) {
		poll_input(); // usb
		printf("[%d]: Hello From CH32X035\n", count++);
		Delay_Ms(100);
	}
}
