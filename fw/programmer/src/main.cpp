#include "pico/time.h"
#include <Arduino.h>
#include <SerialUSB.h>

#define BB_PRINTF_DEBUG(...) Serial.printf(__VA_ARGS__)
//#define BB_PRINTF_DEBUG(...)
#include "bitbang_rvswdio.h"
#include "bitbang_rvswdio_pico.h"

SWIOState state = {.opmode = 2};

void setup() {
    Serial.begin(115200);
	Serial.setTimeout(100);
}


void setup1()
{
	pinMode(LED_BUILTIN, OUTPUT);
}


void loop() {
    if (Serial.available() > 0) {
        char cmd = Serial.read();
        int ret = 0;
        uint8_t reg;

        switch (cmd) {
        case '!':
        	state = {};
        	ret = InitializeSWDSWIO(&state);
         	if (ret != 0) {
          		Serial.printf("error initializing SWD: %d\n", ret);
            	break;
          	}
          	ret = DetermineChipTypeAndSectorInfo(&state, NULL);
            if (ret != 0) {
          		Serial.printf("failed to determine chip type: %d\n", ret);
            	break;
            }
            Serial.printf("chip type is %x\n", state.target_chip_type);
        	break;
        default:
        	Serial.printf("unknown command '%c'\n", cmd);
         	break;
        }
    }
}


void loop1()
{
	digitalWrite(LED_BUILTIN, 1);
	delay(200);
	digitalWrite(LED_BUILTIN, 0);
	delay(200);
}
