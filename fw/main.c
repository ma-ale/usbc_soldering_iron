#include <ch32fun.h>
#include <stdint.h>
#include <stdio.h>
#include <fsusb.h>

#define FR_LEAN
// #define FR_CORE_ONLY
#include <FR_math.h>

#include "funconfig.h"
#include "lib_i2c.h"
#include "display.h"
#include "filter.h"
#include "sc7a20.h"
#include "pd.h"

// Radix for fixed point operations
#define R 16

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
struct pd_profile_t pd_profile;
static const int8_t x_off = 0;
static const int8_t y_off = 8;

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
	(void)numbytes;
	(void)data;
}


/*
 *    __      ____      ____
 * A:   |____|    |____|    |____
 *         ____      ____      __
 * B: ____|    |____|    |____|
 *      ^ ^  ^ ^  ^ ^  ^ ^  ^ ^
 *      f r  r f  f r  r f  f r
 */
void update_encoder(void)
{
	// 0 = 00, 1 = 01, 2 = 10, 3 = 11
	static uint8_t last_state = 0;
	static int8_t count;
	// Lookup table: state_table[last_state][current_state]
	//  0 = invalid move or bounce (ignore)
	//  1 = valid forward step
	// -1 = valid backward step
	static const int8_t state_table[4][4] = {
		{ 0,  1, -1,  0}, // from 00
		{-1,  0,  0,  1}, // from 01
		{ 1,  0,  0, -1}, // from 10
		{ 0, -1,  1,  0}  // from 11
	};

	// FIXME: even with interrupts disabled and debounce, a single detent triggers
	// multiple interrupts, leading to multiple encoder updates in a short time.
	if (funDigitalRead(PIN_BTN) == 0) return;
	bool a = funDigitalRead(PIN_ENC_A);
	bool b = funDigitalRead(PIN_ENC_B);
	Delay_Us(100);
	if (a != funDigitalRead(PIN_ENC_A) || b != funDigitalRead(PIN_ENC_B))
		return; // debounce

	uint8_t current_state = (a << 1) | b;

	// Find the movement direction based on transition
	count += state_table[last_state][current_state];
	if (count == 4) encoder++, count = 0;
	if (count == -4) encoder--, count = 0;

	// Save current state for the next interrupt
	last_state = current_state;
}

// Attached to PIN_ENC_A rising and falling edges
void EXTI7_0_IRQHandler(void) __attribute__((interrupt));
void EXTI7_0_IRQHandler(void)
{
	__disable_irq();
	update_encoder();
	EXTI->INTFR = EXTI_Line3;
	__enable_irq();
}

// Attached to PIN_ENC_B rising and falling edges
void EXTI15_8_IRQHandler(void) __attribute__((interrupt));
void EXTI15_8_IRQHandler(void)
{
	__disable_irq();
	update_encoder();
	EXTI->INTFR = EXTI_Line11;
	__enable_irq();
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


// Integer square root (binary search)
// https://en.wikipedia.org/wiki/Integer_square_root
static inline uint16_t isqrt(uint32_t x)
{
	uint16_t l = 0;     // lower bound of the square root
	uint16_t r = x + 1; // upper bound of the square root

	while (l != r - 1) {
		uint32_t m = (l + r) / 2; // midpoint to test
		if (m * m <= x) {
			l = m;
		} else {
			r = m;
		}
	}

	return l;
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
	AFIO->EXTICR1 |= AFIO_EXTICR1_EXTI11_PB; // Port B channel (pin) 11
	EXTI->INTENR |= EXTI_INTENR_MR11; // Enable EXT11
	EXTI->FTENR |= EXTI_FTENR_TR11;  // Falling edge trigger
	EXTI->RTENR |= EXTI_RTENR_TR11;  // Rising edge trigger
	// enable interrupt
	NVIC_EnableIRQ(EXTI15_8_IRQn);

	// PIN_ENC_A is on port B, channel 3
	AFIO->EXTICR1 |= AFIO_EXTICR1_EXTI3_PB; // Port B channel (pin) 3
	EXTI->INTENR |= EXTI_INTENR_MR3; // Enable EXT3
	EXTI->FTENR |= EXTI_FTENR_TR3;  // Falling edge trigger
	EXTI->RTENR |= EXTI_RTENR_TR3;  // Rising edge trigger
	// enable interrupt
	NVIC_EnableIRQ(EXTI7_0_IRQn);

	Delay_Ms(500);

	u8g2 = display_init();
	sc7a20_init();

	u8g2_ClearBuffer(u8g2);
	u8g2_SetFont(u8g2, u8g2_font_5x8_tr);
	u8g2_DrawStr(u8g2, x_off+0, y_off+7, "Negotiating...");
	u8g2_SendBuffer(u8g2);

	// Init USBPD
	bool has_pd = pd_negotiate(eUSBPD_VCC_3V3);
	if (has_pd == false) {
		u8g2_ClearBuffer(u8g2);
		u8g2_SetFont(u8g2, u8g2_font_5x8_tr);
		u8g2_DrawStr(u8g2, x_off+0, y_off+7, "Negotiation FAILED");
		u8g2_DrawStr(u8g2, x_off+0, y_off+14, USBPD_ResultToStr(pd_get_result()));
		u8g2_SendBuffer(u8g2);
		Delay_Ms(5000);
	} else {
		pd_get_profile(&pd_profile, 60);

		// TODO: let the user decide the power profile
		pd_profile.set_temp = 200;
		pd_profile.set_power = 60; // Slightly below max power to avoid overloading
		pd_profile.tip_r = 2500; // TODO: tip check and resistance calculator
		pd_profile.max_duty = MIN(
			(100*(u32)isqrt((
				MIN(pd_profile.set_power, pd_profile.power_avail)*pd_profile.tip_r)/1000) * 1000) / pd_profile.voltage
			, 100);

		u8g2_ClearBuffer(u8g2);
		u8g2_SetFont(u8g2, u8g2_font_5x8_tr);
		// Display tip temperature
		u8g2_DrawStr(u8g2, x_off+0, y_off+7, "A:");
		u8g2_DrawStr(u8g2, x_off+10, y_off+7, u8g2_u16toa(pd_profile.max_current, 4));
		// Display bus voltage
		u8g2_DrawStr(u8g2, x_off+45, y_off+7, "V:");
		u8g2_DrawStr(u8g2, x_off+55, y_off+7, u8g2_u16toa(pd_profile.voltage, 5));
		// Display power
		u8g2_DrawStr(u8g2, x_off+0, y_off+15, "W:");
		u8g2_DrawStr(u8g2, x_off+10, y_off+15, u8g2_u16toa(pd_profile.power_avail, 3));
		// Display current
		u8g2_DrawStr(u8g2, x_off+45, y_off+15, "D:");
		u8g2_DrawStr(u8g2, x_off+55, y_off+15, u8g2_u16toa(pd_profile.max_duty, 3));
		u8g2_SendBuffer(u8g2);
		// Delay_Ms(5000);
	}


	enum {
		STATE_MENU,
		STATE_HEATING,
	} state = STATE_MENU;
	for (;;) {
		u32 start = funSysTick32();
		poll_input(); // usb

		u8g2_ClearBuffer(u8g2);
		u8g2_SetFont(u8g2, u8g2_font_5x8_tr);

		static bool pwm = false; // PWM status
		static bool enabled = false; // Power electronics enabled
		static uint8_t count = 0; // Loop cycles with PWM on
		static s32 elapsed = 0;
		static bool prev_btn = true;
		bool btn = funDigitalRead(PIN_BTN);

		if (has_pd) {
			static uint16_t vbus_mv, current_ma;
			static volatile int16_t temp_c, tip_temp_c;
			static uint16_t power;
			static s32 e;
			vbus_mv = U16_FP_EMA_K4(vbus_mv, ((u32)adc_buffer[0]*VCC_MV*11)/4096);
			current_ma = U16_FP_EMA_K4(current_ma, get_current_ma(adc_buffer[1]));
			temp_c = I16_FP_EMA_K4(temp_c, get_temp_c(adc_buffer[2]));
			power = ((u32)vbus_mv*current_ma)/1000000;

			// Update the tip temperature only when the PWM is not running
			if (!pwm || !enabled) {
				Delay_Ms(TURN_OFF_DELAY);
				adc_injection_conversion();
				u32 tip_mv = ((u32)injection_results[0]*VCC_MV)/4096;
		    	tip_temp_c = I16_FP_EMA_K2(tip_temp_c, (tip_mv*TC_CONV_NOM)/TC_CONV_DEN) + temp_c;
			}
			int16_t delta = (int16_t)pd_profile.set_temp - tip_temp_c;

			switch (state) {
			case STATE_MENU:
				u8g2_DrawStr(u8g2, x_off+0, y_off+7, "TEMP:");
				u8g2_DrawStr(u8g2, x_off+25, y_off+7, u8g2_u16toa(pd_profile.set_temp, 4));

				if (encoder != 0) {
					pd_profile.set_temp += encoder;
					encoder = 0;
					#define MIN_TEMP 100
					#define MAX_TEMP 360
					if (pd_profile.set_temp < MIN_TEMP) pd_profile.set_temp = MIN_TEMP;
					if (pd_profile.set_temp > MAX_TEMP) pd_profile.set_temp = MAX_TEMP;
				}

				if (btn == 0 && prev_btn == 1) {
					state = STATE_HEATING;
					enabled = false;
				}
				break;
			case STATE_HEATING:
				// Display tip temperature
				u8g2_DrawStr(u8g2, x_off+0, y_off+7, "TIP:");
				u8g2_DrawStr(u8g2, x_off+20, y_off+7, u16toa(tip_temp_c));
				// Display bus voltage
				u8g2_DrawStr(u8g2, x_off+45, y_off+7, "V:");
				u8g2_DrawStr(u8g2, x_off+55, y_off+7, u16toa(vbus_mv/1000));
				// Display power
				//u8g2_DrawStr(u8g2, x_off+0, y_off+15, "W:");
				//u8g2_DrawStr(u8g2, x_off+10, y_off+15, u8g2_u16toa(power, 3));
				FR_printNumF(buf_putc, e, R, 0, 3);
				u8g2_DrawStr(u8g2, x_off+0, y_off+15, buf_get());
				// Display current
				// u8g2_DrawStr(u8g2, x_off+45, y_off+15, "A:");
				// u8g2_DrawStr(u8g2, x_off+55, y_off+15, u16toa(current_ma));
				u8g2_DrawStr(u8g2, x_off+45, y_off+15, "W:");
				u8g2_DrawStr(u8g2, x_off+55, y_off+15, u16toa(power));


				if (enabled) {
					funDigitalWrite(PIN_12V, 1);

					if (count > CYCLES_PER_MEASURE) {
						pwm = false;
						count = 0;
					} else {
						pwm = true;
						count++;
					}

					// Safety logic
					if (current_ma > pd_profile.max_current + pd_profile.max_current/10) {
						pwm = false;
					}
					if (temp_c > MAX_BOARD_TEMP) {
						enabled = false;
						pwm = false;
					}
					if (tip_temp_c > MAX_TIP_TEMP) {
						enabled = false;
						pwm = false;
					}

					if (pwm) {
						const uint16_t tim_max = FUNCONF_SYSTEM_CORE_CLOCK / PWM_FREQ_HZ - 1;
						static s32 err_p, err_i, err_d, dt, fpt, prev_fpt;
						static u32 t, prev_t;
						uint16_t duty;
						// absolute temperature error
						s32 err = I2FR(delta, R);
						fpt = I2FR(tip_temp_c, R);
						t = funSysTick32();

						dt = FR_DIV(I2FR((t-prev_t)/DELAY_MS_TIME, R), R, I2FR(1000, R), R);
						err_p = err;
						// Conditional integration
						if (delta > 40 && delta < -40) err_i = FR_CLAMP(FR_FixAddSat(err_i, FR_FixMulSat(err, dt)), I2FR(-1000, R), I2FR(1000, R));
						err_d = FR_DIV(FR_FixAddSat(prev_fpt, -fpt), R, dt, R);
						prev_t = t;
						prev_fpt = fpt;

						const s32 kp = FR_numstr("1.2000", R);
						const s32 ki = FR_numstr("0.05000", R);
						const s32 kd = FR_numstr("0.00000", R);
						e = 0;
						e = FR_FixAddSat(e, FR_FixMulSat(err_p, kp));
						e = FR_FixAddSat(e, FR_FixMulSat(err_i, ki));
						e = FR_FixAddSat(e, FR_FixMulSat(err_d, kd));

						e = FR_CLAMP(e, I2FR(0, R), I2FR(100, R));
						duty = FR2I(FR_FixMulSat(FR_DIV(e, R, I2FR(100, R), R), I2FR(pd_profile.max_duty, R)), R);

						if (duty <= 100) {
						    pwm_set(((u32)duty*tim_max)/100);
							u8g2_DrawBox(u8g2, x_off+92, y_off+12, 4, 4);
						}
					} else {
						pwm_set(0);
					}
				} else {
					funDigitalWrite(PIN_12V, 0);
				}

				// move to menu mode when encoder is turned
				if (encoder != 0) {
					state = STATE_MENU;
					funDigitalWrite(PIN_12V, 0);
					pwm_set(0);
				}

				// Check button to toggle enable
				if (btn == 0 && prev_btn == 1) {
					enabled = !enabled;
					if (enabled) {
						pwm_on();
					} else {
					    e = 0;
						pwm_off();
					}
				}
				break;
			}
		} else {
			// No PD capability, just display a message
			u8g2_DrawStr(u8g2, x_off+0, y_off+7, "NO PD");
		}

		u8g2_SendBuffer(u8g2);

		prev_btn = btn;

		elapsed = funSysTick32() - start;
		if (elapsed > 0 && elapsed < Ticks_from_Ms(FRAME_TIME_MS)) {
			DelaySysTick(Ticks_from_Ms(FRAME_TIME_MS) - elapsed);
		}
	}
}
