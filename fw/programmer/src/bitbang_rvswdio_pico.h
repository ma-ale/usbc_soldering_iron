#include <hardware/gpio.h>
#include <pico/time.h>

#include "bitbang_rvswdio.h"
#include "hardware/timer.h"
#include "lwipopts.h"
#include "pico.h"


#define PIN_SWDIO 10
#define PIN_SWCLK 11


// open drain emulation, the pin is set with output '0' and is switched between input or output
// depending on the wanted value, in the high state the line is pulled high by the pull-up and
// in the low state the line in forced low
#define OD_PULL(pin, value) gpio_set_dir((pin), (value) ? GPIO_IN : GPIO_OUT)

// wait time between line transitions
#define SWD_DELAY() busy_wait_us(1);

// microseconds between each register read/write
#define STOP_WAIT 8

// Single wire debug (SWDIO and SWCLK)
static inline void ConfigureIOForRVSWD(void)
{
	// SWDIO, open drain (emulated) with pull-up
	gpio_init(PIN_SWDIO);
    gpio_set_pulls(PIN_SWDIO, true, false);
    gpio_put(PIN_SWDIO, 0);
    gpio_set_dir(PIN_SWDIO, GPIO_IN);

    gpio_init(PIN_SWCLK);
    gpio_set_pulls(PIN_SWCLK, false, false);
    gpio_put(PIN_SWCLK, 0);
    gpio_set_dir(PIN_SWCLK, GPIO_OUT);
}


// Single wire input-output SDI (just SWDIO)
static inline void ConfigureIOForRVSWIO(void)
{
	BB_PRINTF_DEBUG( "TODO: add support for SWIO\n" );
}


static inline void rvswd_start(void)
{
    // Start with both lines high
    gpio_put(PIN_SWCLK, 1);
    OD_PULL(PIN_SWDIO, 1);
    //SWD_DELAY();

    // Pull data low
    OD_PULL(PIN_SWDIO, 0);
    SWD_DELAY();
}


static inline void rvswd_stop(void)
{
    gpio_put(PIN_SWCLK, 0);
    SWD_DELAY();

    OD_PULL(PIN_SWDIO, 0);
    SWD_DELAY();

    gpio_put(PIN_SWCLK, 1);
    SWD_DELAY();

    OD_PULL(PIN_SWDIO, 1);
}


void rvswd_write_bit(bool value)
{
    gpio_put(PIN_SWCLK, 0);
    OD_PULL(PIN_SWDIO, value);
    SWD_DELAY();
    gpio_put(PIN_SWCLK, 1);  // Data is sampled on rising edge of clock
    SWD_DELAY();
}


bool rvswd_read_bit(void)
{
    OD_PULL(PIN_SWDIO, 0);
    gpio_put(PIN_SWCLK, 0);
    OD_PULL(PIN_SWDIO, 1);
    SWD_DELAY();
    bool bit = gpio_get(PIN_SWDIO);

    gpio_put(PIN_SWCLK, 1);  // Data is output on rising edge of clock
    SWD_DELAY();
    return bit;
}


static void MCFWriteReg32( struct SWIOState * state, uint8_t command, uint32_t value )
{
	// only supported mode is SWD
	if (state->opmode != 2) {
		BB_PRINTF_DEBUG( "TODO: add support for SWIO\n" );
		return;
	}

	noInterrupts();
	rvswd_start();

    // ADDR HOST
    bool parity = false;  // This time it's odd parity?
    for (uint8_t position = 0; position < 7; position++) {
        bool bit = (command >> (6 - position)) & 1;
        rvswd_write_bit(bit);
        if (bit) parity = !parity;
    }

    // Operation: write
    rvswd_write_bit(1);
    parity = !parity;

    // Parity bit (even)
    rvswd_write_bit(parity);

    rvswd_read_bit(); // ???
	rvswd_read_bit(); // Seems only need to be set for first transaction (We are ignoring that though)
	rvswd_read_bit(); // ???
	rvswd_write_bit(0); // 0 for register, 1 for value.
	rvswd_write_bit(0); // ???  Seems to have something to do with halting.

    // Data
    parity = false;  // This time it's even parity?
    for (uint8_t position = 0; position < 32; position++) {
        bool bit = (value >> (31 - position)) & 1;
        rvswd_write_bit(bit);
        if (bit) parity = !parity;
    }

    // Parity bit
    rvswd_write_bit(parity);

    rvswd_read_bit(); // ???
	rvswd_read_bit(); // ???
	rvswd_read_bit(); // ???
	rvswd_write_bit(1); // 0 for register, 1 for value
	rvswd_write_bit(0); // ??? Seems to have something to do with halting?

    rvswd_stop();
    interrupts();
    sleep_us(STOP_WAIT);
}


static int MCFReadReg32( struct SWIOState * state, uint8_t command, uint32_t * value )
{
	// only supported mode is SWD
	if (state->opmode != 2) {
		BB_PRINTF_DEBUG( "TODO: add support for SWIO\n" );
		return -1;
	}

	bool parity;

	noInterrupts();
    rvswd_start();

    // ADDR HOST
    parity = false;
    for (uint8_t position = 0; position < 7; position++) {
        bool bit = (command >> (6 - position)) & 1;
        rvswd_write_bit(bit);
        if (bit) parity = !parity;
    }

    // Operation: read
    rvswd_write_bit(0);

    // Parity bit (even)
    rvswd_write_bit(parity);

    rvswd_read_bit(); // ??
    rvswd_read_bit(); // ??
    rvswd_read_bit(); // ??

    rvswd_write_bit(0); // 0 for register, 1 for value
    rvswd_write_bit(0); // ??? Seems to have something to do with halting?

    *value = 0;

    // Data
    parity = false;
    uint32_t rval = 0;
    for (uint8_t position = 0; position < 32; position++) {
        bool bit = rvswd_read_bit();
        rval <<= 1;
        if (bit) {
        	rval |= 1;
         	parity ^= 1;
        }
    }
    *value = rval;

    // Parity bit
    bool parity_read = rvswd_read_bit();

    rvswd_read_bit(); // ??
    rvswd_read_bit(); // ??
    rvswd_read_bit(); // ??

    rvswd_write_bit(1); // 1 for data
	rvswd_write_bit(0); // ??? Seems to have something to do with halting?

    rvswd_stop();
    interrupts();

    sleep_us(STOP_WAIT);
    return (parity == parity_read) ? 0 : -1;
}
