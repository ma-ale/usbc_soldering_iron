	#include "pico/time.h"
	#include <Arduino.h>
	#include <SerialUSB.h>

	//#define BB_PRINTF_DEBUG(...) Serial.printf(__VA_ARGS__)
	#define BB_PRINTF_DEBUG(...)
	#include "bitbang_rvswdio.h"
	#include "bitbang_rvswdio_pico.h"

	#define PROTOCOL_START     '!'
	#define PROTOCOL_ACK       '+'
	#define PROTOCOL_TEST      '?'
	#define PROTOCOL_POWER_ON  'p'
	#define PROTOCOL_POWER_OFF 'P'
	#define PROTOCOL_WRITE_REG 'w'
	#define PROTOCOL_READ_REG  'r'

	bool last_dtr = false;
	SWIOState state = {.opmode = 2};

	void setup() {
	    Serial.begin(115200);
		Serial.setTimeout(100);
	    ConfigureIOForRVSWD();

	    // We do NOT block waiting for Serial here,
	    // we handle the connection dynamically in the loop.
	}


	void setup1()
	{
		pinMode(LED_BUILTIN, OUTPUT);
	}


	void loop() {
	    // Monitor DTR line to simulate Arduino Reset behavior
	    bool current_dtr = Serial.dtr();
	    if (current_dtr && !last_dtr) {
	        // minichlink just opened the port
	        ConfigureIOForRVSWD();
			while (Serial.available() > 0) { Serial.read(); }
			delay(100);
	        Serial.write(PROTOCOL_START); // Announce readiness
	    }
	    last_dtr = current_dtr;

	    if (Serial.available() > 0) {
	        char cmd = Serial.read();
	        uint8_t reg;
	        uint32_t val;

	        switch (cmd) {
	            case PROTOCOL_TEST:
	                Serial.write(PROTOCOL_ACK);
	                break;
	            case PROTOCOL_POWER_ON:
	                // Not needed for rvswd
	                sleep_us(10);
	                Serial.write(PROTOCOL_ACK);
	                break;
	            case PROTOCOL_POWER_OFF:
	                // Not needed for rvswd
	                sleep_us(10);
	                Serial.write(PROTOCOL_ACK);
	                break;
	            case PROTOCOL_WRITE_REG:
	                if (Serial.readBytes((char*)&reg, sizeof(uint8_t)) != 1) break;
	                if (Serial.readBytes((char*)&val, sizeof(uint32_t)) != 4) break;
	                MCFWriteReg32(&state, reg, val);
	                Serial.write(PROTOCOL_ACK);
	                break;
	            case PROTOCOL_READ_REG:
	                if (Serial.readBytes((char*)&reg, sizeof(uint8_t)) != 1) break;
	                MCFReadReg32(&state, reg, &val);
	                Serial.write((char*)&val, sizeof(uint32_t));
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
