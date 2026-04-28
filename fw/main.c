#include <ch32fun.h>
#include <stdint.h>
#include <stdio.h>
#include <fsusb.h>

#define USBPD_IMPLEMENTATION
#include "usbpd.h"

#include "lib_i2c.h"
#include "display.h"
#include "filter.h"
#include "sc7a20.h"


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

#define FRAME_TIME_MS 41 // roughly 24 fps

#define PWM_FREQ_HZ 150000

// constants
// LUT for converting NTC readings to degrees celsius
// Nominal: 1kOhm, Beta: 3380, Step: 64
const uint8_t ntc_step_size = 64;
const int16_t ntc_lut[] = {
	1316, 197, 155, 133, 119, 108, 100, 93, 87, 82, 77, 73, 69, 66, 63, 60,
	57, 54, 52, 50, 47, 45, 43, 41, 39, 37, 35, 34, 32, 30, 28, 27,
	25, 23, 22, 20, 19, 17, 15, 14, 12, 11, 9, 7, 6, 4, 2, 0,
	-1, -3, -5, -7, -9, -11, -14, -16, -19, -22, -25, -28, -32, -38, -44, -55,
	-55 // extra value to not have an extra if statement
};


u8g2_t *u8g2;
int16_t encoder = 0; // rotary encoder counter
uint32_t last_interrupt = 0; // last time the encoder interrupt was triggered
#define ENCODER_DEBOUNCE 6000

// TODO: these need to be calibrated
// Tip mV to deg C conversion factor numerator
#define TC_CONV_NOM 151
// Tip mV to deg C conversion factor denumerator
#define TC_CONV_DEN 1000

// Convert the raw adc reading to a temperature in celsius with the ntc lut,
// linearly interpolating between positions
static inline int16_t get_temp_c(uint16_t adc_reading)
{
	if (adc_reading > 4095) return 0;
	uint8_t index = adc_reading / ntc_step_size;
	uint8_t remainder = adc_reading % ntc_step_size;
	int16_t temp_base = index < 64 ? ntc_lut[index] : 0;
    int16_t temp_next = ntc_lut[index + 1];
    return temp_base + ((temp_next - temp_base) * remainder)/ntc_step_size;
}


// convert the raw TPA191 adc reading to a current in milliamps
static inline int16_t get_current_ma(uint16_t adc_reading)
{
	// Rshunt = 4 milliOhm
	// Gain = 100
	u32 mv = ((u32)adc_reading * VCC_MV) / 4096;
	return (mv * 10) / 4;
}


void print_i2c_device(uint8_t addr)
{
	printf("Device found at 0x%02X\n", addr);
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
				"\ts : scan I2C bus\n"
			);
			break;
		case 's':
			printf("Scanning I2C bus...\n");
			i2c_scan(I2C_TARGET, print_i2c_device);
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


// Procedure to get hardware I2C working on the CH32X035F8U6
static inline void setup_i2c(void)
{
	// Order here matters, first initialize the AFIO and I2C subsystems then
	// change register values, do that the other way around and the configuration
	// wont take effect
	// Enable AFIO (Alternate Function IO)
	RCC->APB2PCENR |= RCC_AFIOEN;
	// Init I2C
	i2c_init(I2C_TARGET, FUNCONF_SYSTEM_CORE_CLOCK, 400000);

	// To utilize the I2C bus we need to disable SWD first, since the pins overlap
	AFIO->PCFR1 &= ~(0b0111 << 24);
	AFIO->PCFR1 |=   0b0100 << 24;

	// Map SCL to PC18 and SDA to PC19
	AFIO->PCFR1 |= 0b0101 << 2;
	// Manually set the I2C pins to Alternate Function IO,  CNF=10b, MODE=10b
	GPIOC->CFGXR &= ~((0xF << 8) | (0xF << 12)); // first clear the bits
	// then set them
	GPIOC->CFGXR |= 0b1010 << 8; // PC18
	GPIOC->CFGXR |= 0b1010 << 12; // PC19
}


// FIXME: this holds the max number of ADC channels, ideally this would be lower
volatile uint16_t adc_buffer[16];
// Set-up the adc to perform continous conversion of enabled channels, enable
// channel injection and setup DMA to automatically transfer conversion results
// to a buffer
static inline void setup_adc_and_dma(uint8_t *channels, uint8_t ch_size, uint8_t *injected, uint8_t in_size)
{
	if (channels == NULL || ch_size == 0) return;
	// FIXME: for now only support a single configuration register, so at max 6 channels
	//        I don't want to implement logic to switch registers right now
	// TODO: report an error
	if (ch_size > 6) return;

	// General ADC initialization, I use this only for enabling clocks and such
	funAnalogInit(); // general initialization

	// Set the SCAN and CONT bits to enter continous conversion mode
	// Do not set the IAUTO bit to ensure injected channels are converted only
	// when specified by software
	// Set the regular channel trigger mode to software (EXTSEL = 0b111) to ensure
	// conversion starts in sync with DMA (this should already be done by funAnalogInit())
	ADC1->CTLR1 |= ADC_SCAN;
	ADC1->CTLR2 |= ADC_CONT | ADC_EXTSEL;

	// Setup regular channels (scanned continously)
	// TODO: setup sampling time
	for (uint8_t i = 0; i < ch_size; i++) {
		// Increase sampling time to have less cross talk between channels
		// 0b111 means 11 ADC clock cycles (max)
		if (channels[i] < 10) {
			ADC1->SAMPTR2 |= 0b111 << (channels[i]*3);
		} else {
			ADC1->SAMPTR1 |= 0b111 << ((channels[i]-10)*3);
		}
		ADC1->RSQR3 |= channels[i] << (i*5);
	}
	// Set the number of normal conversion channels
	ADC1->RSQR1 |= (ch_size-1) << 20;

	// Set-up injection channels
	// The injection channel register allows for up to 4 channels, if the buffer is larger
	// skip this step
	// TODO: report an error
	if (injected != NULL && in_size != 0 && in_size <= 4) {
		// enable injection group
		// JEXTSEL = 0b111 (software trigger)
		ADC1->CTLR2 |= ADC_JEXTSEL;
		for (uint8_t i = 0; i < in_size; i++) {
			ADC1->ISQR |= injected[i] << ((4-in_size+i)*5);
		}
		// Set the number of injection channels
		ADC1->ISQR |= (in_size-1) << 20;
	}

	// Set-up DMA, ADC is connected only to channel 1
	// Turn on DMA
	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;
	// 1. Set the address of the source peripheral, in this case the output
	//    register of the ADC
	DMA1_Channel1->PADDR = (uint32_t)&(ADC1->RDATAR);
	// 2. Set the destination (base) address of the data
	DMA1_Channel1->MADDR = (uint32_t)adc_buffer;
	// 3. Set the number of transfers to be done each cycle, in this case it's
	//    the same as the number of regular conversion cycles
	DMA1_Channel1->CNTR = ch_size;
	// 4. Set mode: Peripheral to Memory, MEM2MEM=0 DIR=0
	//    Circular mode CIRC=1, restart after CNTR transfers
	//    Max priority to not lose conversions and offset values in the buffer
	DMA1_Channel1->CFGR  =
		DMA_M2M_Disable |
		DMA_Priority_VeryHigh |
		DMA_MemoryDataSize_HalfWord |
		DMA_PeripheralDataSize_HalfWord |
		DMA_MemoryInc_Enable |
		DMA_Mode_Circular |
		DMA_DIR_PeripheralSRC;
	// Turn on DMA channel 1
	DMA1_Channel1->CFGR |= DMA_CFGR1_EN;

	// Enable DMA requests from the ADC
	ADC1->CTLR2 |= ADC_DMA;
	// Power up ADC again
	ADC1->CTLR2 |= ADC_ADON;
	// start conversion
	ADC1->CTLR2 |= ADC_SWSTART;
}


// Perform a conversion on the injected ADC channels, this is synchronous and
// halts conversion on the normal channels until it completes
volatile uint16_t injection_results[4];
static inline bool adc_injection_conversion() {
	// Clear any pending flags
	ADC1->STATR &= ~(ADC_JEOC);

	// Start injection conversion using external trigger method
	ADC1->CTLR2 |= ADC_JSWSTART;

	// Wait for all conversions to complete
	s32 timeout = 1000000;
	while(!(ADC1->STATR & ADC_JEOC) && timeout--) {}

	if (timeout <= 0) return false;

	// Read all results
	injection_results[0] = ADC1->IDATAR1 & 0x0FFF;
	injection_results[1] = ADC1->IDATAR2 & 0x0FFF;
	injection_results[2] = ADC1->IDATAR3 & 0x0FFF;
	injection_results[3] = ADC1->IDATAR4 & 0x0FFF;

	// Clear JEOC flag
	ADC1->STATR &= ~ADC_JEOC;

	return true;
}

// mask for the CCxP bits
// when set PWM outputs are held HIGH by default and pulled LOW
// when zero PWM outputs are held LOW by default and pulled HIGH
// #define TIM3_DEFAULT 0xff
#define TIM3_DEFAULT 0x00
static inline void setup_pwm(void)
{
	// Enable TIM3 clock
	RCC->APB1PCENR |= RCC_APB1Periph_TIM3;

	// Since TIM3 is enabled with TIM3->SMCFGR.SMS = 0b000 (default value) the
	// clock source is the internal clock
	// The internal clock goes through the HBB prescaler RCC->CFGR0.HPRE which
	// by default is disabled

	// Reset TIM3 to init all regs
	RCC->APB1PRSTR |= RCC_APB1Periph_TIM3;
	RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM3;

	// set TIM3 clock prescaler divider
	TIM3->PSC = 0x0000; // no prescaler
	// set PWM total cycle width
	static_assert((FUNCONF_SYSTEM_CORE_CLOCK / PWM_FREQ_HZ - 1) < 0xffff, "PWM_FREQ_HZ too low for TIM3");
	TIM3->ATRLR = FUNCONF_SYSTEM_CORE_CLOCK / PWM_FREQ_HZ - 1;

	// for channel 1 let CCxS stay 00 (output), set OC1M to 110 (PWM 1)
	// enabling preload (OC1PE) causes the new pulse width in compare capture
	// register only to come into effect when UG bit in SWEVGR is set
	// 	(= initiate update) (auto-clears)
	TIM3->CHCTLR1 |= TIM_OC1M_2 | TIM_OC1M_1 | TIM_OC1PE;

	// Enable channel 1 output (PA6)
	TIM3->CCER |= TIM_CC1E | ( TIM_CC1P & TIM3_DEFAULT );

	// CTLR1: default is up, events generated, edge align
	// enable auto-reload of preload
	TIM3->CTLR1 |= TIM_ARPE;
}


static inline void pwm_on(void)
{
	// initialize counter
	TIM3->SWEVGR |= TIM_UG;
	// Enable TIM3
	TIM3->CTLR1 |= TIM_CEN;
}


static inline void pwm_off(void)
{
	// Disable TIM3
	TIM3->CTLR1 &= ~TIM_CEN;
	TIM3->CH1CVR = 0;
}


static inline void pwm_set(uint16_t pulse_width)
{
	TIM3->CH1CVR = pulse_width;
}


__attribute__((noreturn)) int main(void)
{
	SystemInit();
	funGpioInitAll();
	USBFSSetup();

	funPinMode(PIN_VBUS, GPIO_CFGLR_IN_ANALOG);
	funPinMode(PIN_CURRENT, GPIO_CFGLR_IN_ANALOG);
	funPinMode(PIN_NTC, GPIO_CFGLR_IN_ANALOG);
	funPinMode(PIN_TEMP, GPIO_CFGLR_IN_ANALOG);

	uint8_t adc_channels[3] = {
		VBUS_ADC_CHANNEL,
		CURRENT_ADC_CHANNEL,
		NTC_ADC_CHANNEL
	};
	uint8_t adc_injected[1] = {
		TEMP_ADC_CHANNEL
	};
	setup_adc_and_dma(adc_channels, 3, adc_injected, 1);

	funPinMode(PIN_12V, GPIO_CFGLR_OUT_10Mhz_PP);
	funDigitalWrite(PIN_12V, 0);
	funPinMode(PIN_HEATER, GPIO_CFGLR_OUT_10Mhz_AF_PP);
	funDigitalWrite(PIN_HEATER, 0);
	funPinMode(PIN_DISP_RST, GPIO_CFGLR_OUT_10Mhz_PP);
	funDigitalWrite(PIN_DISP_RST, 1); // start with display disabled

	funPinMode(PIN_ENC_A, GPIO_CFGLR_IN_PUPD); // enable pull-up/down
	funDigitalWrite(PIN_ENC_A, 1); // specify pull-up
	funPinMode(PIN_ENC_B, GPIO_CFGLR_IN_PUPD); // enable pull-up/down
	funDigitalWrite(PIN_ENC_B, 1); // specify pull-up
	funPinMode(PIN_BTN, GPIO_CFGLR_IN_FLOAT);

	setup_pwm();
	pwm_off();

	setup_i2c();

	// Configure the IO as an interrupt.
	// PIN_ENC_B is on port B, channel 11
	AFIO->EXTICR1 = AFIO_EXTICR1_EXTI11_PB; // Port B channel (pin) 11
	EXTI->INTENR = EXTI_INTENR_MR11; // Enable EXT11
	EXTI->FTENR = EXTI_FTENR_TR11;  // Falling edge trigger
	// enable interrupt
	NVIC_EnableIRQ(EXTI15_8_IRQn);

	Delay_Ms(500);

	u8g2 = display_init();
	sc7a20_init();

	// Init USBPD
	USBPD_VCC_e vcc = eUSBPD_VCC_3V3;
	USBPD_Result_e result = USBPD_Init(vcc);
	if (result != eUSBPD_OK) {
		printf("USBPD_Init failed: %d\n", result);
	}

	bool has_pd = false;
	USBPD_SPR_CapabilitiesMessage_t *capabilities = NULL;
	uint32_t cap_count = 0;
	int max_v = 5;
	int idx_9v = -1;
	u32 start = funSysTick32();
	while (eUSBPD_BUSY == (result = USBPD_SinkNegotiate())) {
		u32 now = funSysTick32();
		if (now - start > Ticks_from_Ms(5000)) {
			printf("USBPD_SinkNegotiate timed out\n");
			break;
		}

		u8g2_ClearBuffer(u8g2);
		u8g2_SetBitmapMode(u8g2, 1);
		u8g2_SetFontMode(u8g2, 1);
		u8g2_SetFont(u8g2, u8g2_font_5x8_tr);
		u8g2_DrawStr(u8g2, 0, 18, "waiting...");
		u8g2_SendBuffer(u8g2);
	}
	if (result != eUSBPD_OK) {
		printf("USBPD_SinkNegotiate failed: %s, state: %s\n",
			USBPD_ResultToStr(result),
			USBPD_StateToStr(USBPD_GetState())
		);
	} else {
		has_pd = true;
		cap_count = USBPD_GetCapabilities(&capabilities);
		for (u32 i = 0; i < cap_count; i++) {
			USBPD_SinkPDO_t *pdo = &capabilities->Sink[i];
			switch (pdo->Header.PDOType) {
			case eUSBPD_PDO_FIXED:
				if (pdo->FixedSupply.VoltageIn50mV/20 > max_v) {
					max_v = pdo->FixedSupply.VoltageIn50mV/20;
				}
				if (pdo->FixedSupply.VoltageIn50mV/20 == 9) {
					idx_9v = i;
				}
				break;
			case eUSBPD_PDO_BATTERY:
				if (pdo->BatterySupply.MaxVoltageIn50mV/20 > max_v) {
					max_v = pdo->BatterySupply.MaxVoltageIn50mV/20;
				}
				break;
			case eUSBPD_PDO_VARIABLE:
				if (pdo->VariableSupply.MaxVoltageIn50mV/20 > max_v) {
					max_v = pdo->VariableSupply.MaxVoltageIn50mV/20;
				}
				break;
			case eUSBPD_PDO_AUGMENTED:
				// TODO
				break;
			}
		}
	}

	if (idx_9v >= 0) {
		USBPD_SelectPDO(idx_9v, 0);
		Delay_Ms(200);
	}

	for (;;) {
		static uint16_t vbus_mv, current_ma;
		static int16_t temp_c, tip_temp_c;
		u32 start = funSysTick32();

		poll_input(); // usb

		vbus_mv = U16_FP_EMA_K4(vbus_mv, ((u32)adc_buffer[0]*VCC_MV*11)/4096);
		current_ma = U16_FP_EMA_K4(current_ma, get_current_ma(adc_buffer[1]));
		temp_c = I16_FP_EMA_K4(temp_c, get_temp_c(adc_buffer[2]));
		if (!adc_injection_conversion()) {
		    printf("injection conversion failed");
		} else {
			u32 tip_mv = ((u32)injection_results[0]*VCC_MV)/4096;
		    tip_temp_c = I16_FP_EMA_K2(tip_temp_c, (tip_mv*TC_CONV_NOM)/TC_CONV_DEN) + temp_c;
		}

		static bool pwm = false;
		if (funDigitalRead(PIN_BTN) == 0) {
			if (!pwm) {
				funDigitalWrite(PIN_12V, 1);
				pwm_on();
				pwm = true;
				pwm_set(100);
			}
		} else {
			funDigitalWrite(PIN_12V, 0);
			pwm_off();
			pwm = false;
		}

		u8g2_ClearBuffer(u8g2);
		u8g2_SetBitmapMode(u8g2, 1);
		u8g2_SetFontMode(u8g2, 1);
		u8g2_SetFont(u8g2, u8g2_font_5x8_tr);
#define x_off 0
#define y_off 8
		static bool mode = true;
//		if (mode) {
			u8g2_DrawStr(u8g2, x_off+0, y_off+7, "TIP:");
			u8g2_DrawStr(u8g2, x_off+20, y_off+7, u8x8_u16toa(tip_temp_c, 4));
			u8g2_DrawStr(u8g2, x_off+0, y_off+15, "VBUS:");
			u8g2_DrawStr(u8g2, x_off+25, y_off+15, u8x8_u16toa(vbus_mv, 4));
			u8g2_DrawStr(u8g2, x_off+51, y_off+7, "TEMP:");
			u8g2_DrawStr(u8g2, x_off+75, y_off+7, u8x8_u16toa(temp_c, 2));
			//u8g2_DrawStr(u8g2, x_off+51, y_off+15, "V:");
			//u8g2_DrawStr(u8g2, x_off+60, y_off+15, u8x8_u16toa(max_v, 2));
			u8g2_DrawStr(u8g2, x_off+50, y_off+15, "CURR:");
			u8g2_DrawStr(u8g2, x_off+75, y_off+15, u8x8_u16toa(current_ma, 4));
//		} else {
//			static int16_t ax, ay, az;
//			sc7a20_get_readings(&ax, &ay, &az);
//			u8g2_DrawStr(u8g2, x_off+0, y_off+7, ax > 0 ? "AX:+" : "AX:-");
//			u8g2_DrawStr(u8g2, x_off+20, y_off+7, u8x8_u16toa(ax > 0 ? ax : -ax, 5));
//			u8g2_DrawStr(u8g2, x_off+0, y_off+15, ay > 0 ? "AY:+" : "AY:-");
//			u8g2_DrawStr(u8g2, x_off+20, y_off+15, u8x8_u16toa(ay > 0 ? ay : -ay, 5));
//			u8g2_DrawStr(u8g2, x_off+50, y_off+7, az > 0 ? "AZ:+" : "AZ:-");
//			u8g2_DrawStr(u8g2, x_off+70, y_off+7, u8x8_u16toa(az > 0 ? az : -az, 5));
//		}

		if (pwm) {
			u8g2_DrawBox(u8g2, x_off+92, y_off+0, 4, 4);
		}

		u8g2_SendBuffer(u8g2);

/*
		if (idx_9v != -1 && funDigitalRead(PIN_BTN) == 0) {
			USBPD_SelectPDO(idx_9v, 0);
			Delay_Ms(200);
		}
*/
		if (encoder != 0) {
			mode = !mode;
			encoder = 0;
		}

//		printf("VBUS=%d, CURRENT=%d, TEMP=%d, TIP=%d, COUNTER=%d\n", vbus_mv, current_ma, temp_k, tip_mv, encoder);

		u32 elapsed = funSysTick32() - start;
		if (elapsed < Ticks_from_Ms(FRAME_TIME_MS)) {
			DelaySysTick(Ticks_from_Ms(FRAME_TIME_MS) - elapsed);
		} else {
			printf("Frame took too long: %ld ms\n", elapsed/DELAY_MS_TIME);
		}
	}
}
