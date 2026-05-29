#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

#define FUNCONF_USE_DEBUGPRINTF 0 // can only have one printf
#define FUNCONF_USE_USBPRINTF   1
#define FUNCONF_DEBUG_HARDFAULT 0
#define CH32X035                1
#define I2C_TARGET              I2C1
#define FRAME_TIME_MS           20 // 50Hz
#define PWM_FREQ_HZ             100000 // TIM3 PWM frequency

// Pin definitions
#define PIN_VBUS     PA0  // vbus voltage feedback
#define PIN_CURRENT  PA1  // current feedback
#define PIN_NTC      PA2  // ntc temperature sensor
#define PIN_TEMP     PA3  // thermocouple amplifier
#define PIN_12V      PA5  // 12V regulator enable
#define PIN_HEATER   PA6  // power mosfet gate control
#define PIN_ENC_A    PB3  // rotary encoder A
#define PIN_ENC_B    PB11 // rotary encoder B
#define PIN_BTN      PB1  // rotary encoder button

// Analog channel definitions
#define VBUS_ADC_CHANNEL    ANALOG_0 // PA0
#define CURRENT_ADC_CHANNEL ANALOG_1 // PA1
#define NTC_ADC_CHANNEL     ANALOG_2 // PA2
#define TEMP_ADC_CHANNEL    ANALOG_3 // PA3
#define VREF_INT_CHANNEL    15       // Internal channel set to 1.2V reference

#define ENCODER_INV true
#define ENCODER_FAST 3

#define MAX_BOARD_TEMP 50
#define MAX_TIP_TEMP   500
#define TURN_OFF_DELAY 3
#define CYCLES_PER_MEASURE 2
#define BOARD_MAX_VOLTAGE 28000
#define MIN_TIP_SET_TEMP 100
#define MAX_TIP_SET_TEMP 420



#endif // _FUNCONFIG_H
