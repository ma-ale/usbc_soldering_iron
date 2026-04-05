#include <hardware/gpio.h>
#include <pico/time.h>

#include "bitbang_rvswdio.h"
#include "hardware/timer.h"
#include "lwipopts.h"
#include "pico.h"


#define PIN_SWDIO 10
#define PIN_SWCLK 11


// wait time between line transitions
#define SWD_DELAY() busy_wait_us(2);

// microseconds between each register read/write
#define STOP_WAIT 8

// Single wire debug (SWDIO and SWCLK)
static inline void ConfigureIOForRVSWD(void)
{
    // SWCLK forced, starts at 1
	gpio_init(PIN_SWCLK);
    gpio_set_pulls(PIN_SWCLK, false, false);
    gpio_put(PIN_SWCLK, 1);
    gpio_set_dir(PIN_SWCLK, GPIO_OUT);

    // SWDIO, open drain (emulated) with pull-up
	gpio_init(PIN_SWDIO);
    gpio_set_pulls(PIN_SWDIO, true, false);
    gpio_put(PIN_SWDIO, 1);
    gpio_set_dir(PIN_SWDIO, GPIO_IN);
}


// Single wire input-output SDI (just SWDIO)
static inline void ConfigureIOForRVSWIO(void)
{
	BB_PRINTF_DEBUG( "TODO: add support for SWIO\n" );
}


void rvswd_write_bit(bool value)
{
    gpio_put(PIN_SWCLK, 0);
    gpio_put(PIN_SWDIO, value);
    gpio_set_dir(PIN_SWDIO, GPIO_OUT);
    gpio_set_dir(PIN_SWDIO, GPIO_IN);
    SWD_DELAY();
    gpio_put(PIN_SWCLK, 1);  // Data is sampled on rising edge of clock
    SWD_DELAY();
}


bool rvswd_read_bit(void)
{
	gpio_put(PIN_SWDIO, 1);
	gpio_set_dir(PIN_SWDIO, GPIO_IN);
    gpio_put(PIN_SWCLK, 0);
    SWD_DELAY();
    bool bit = gpio_get(PIN_SWDIO);
    gpio_put(PIN_SWCLK, 1);  // Data is output on rising edge of clock
    SWD_DELAY();
    return bit;
}


static inline void rvswd_stop(void)
{
    gpio_put(PIN_SWCLK, 0);
    SWD_DELAY();
    gpio_put(PIN_SWDIO, 0);
    gpio_set_dir(PIN_SWDIO, GPIO_OUT);
    SWD_DELAY();
    gpio_put(PIN_SWCLK, 1);
    SWD_DELAY();
    gpio_put(PIN_SWDIO, 1);
    gpio_set_dir(PIN_SWDIO, GPIO_IN);
}


static void MCFWriteReg32( struct SWIOState * state, uint8_t command, uint32_t value )
{
	// only supported mode is SWD
	if (state->opmode != 2) {
		BB_PRINTF_DEBUG( "TODO: add support for SWIO\n" );
		return;
	}

	noInterrupts();

	// start transaction
	gpio_put(PIN_SWDIO, 0);
	gpio_set_dir(PIN_SWDIO, GPIO_OUT);
	SWD_DELAY();

    // ADDR HOST
    bool parity = true;
    for (uint32_t mask = 1<<6; mask; mask >>= 1) {
        bool bit = !!(command & mask);
        parity ^= bit;
        rvswd_write_bit(bit);
    }

    rvswd_write_bit(1); // Operation: write
    rvswd_write_bit(parity);
    rvswd_read_bit(); // ???
	rvswd_read_bit(); // Seems only need to be set for first transaction (We are ignoring that though)
	rvswd_read_bit(); // ???
	rvswd_write_bit(0); // 0 for register, 1 for value.
	rvswd_write_bit(0); // ???  Seems to have something to do with halting.

    // DATA
    parity = false;  // This time it's even parity?
    for (uint32_t mask = 1<<31; mask; mask >>= 1) {
        bool bit = !!(value & mask);
        parity ^= bit;
        rvswd_write_bit(bit);
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

	noInterrupts();

	// start transaction
	gpio_put(PIN_SWDIO, 0);
	gpio_set_dir(PIN_SWDIO, GPIO_OUT);
	SWD_DELAY();

    // ADDR HOST
    bool parity = false;
    for (uint8_t mask = 1<<6; mask; mask >>= 1) {
        bool bit = !!(command & mask);
        parity ^= bit;
        rvswd_write_bit(bit);
    }

    rvswd_write_bit(0); // Operation: read
    rvswd_write_bit(parity);
    rvswd_read_bit(); // ??
    rvswd_read_bit(); // ??
    rvswd_read_bit(); // ??
    rvswd_write_bit(0); // 0 for register, 1 for value
    rvswd_write_bit(0); // ??? Seems to have something to do with halting?

    // DATA
    *value = 0;
    parity = false;
    uint32_t rval = 0;
    for (uint8_t position = 0; position < 32; position++) {
        bool bit = rvswd_read_bit();
        rval <<= 1;
       	rval |= bit;
       	parity ^= bit;
    }
    *value = rval;

    // Parity bit
    bool parity_read = rvswd_read_bit();
    if (parity_read != parity) goto read_end;

    rvswd_read_bit(); // ??
    rvswd_read_bit(); // ??
    rvswd_read_bit(); // ??
    rvswd_write_bit(1); // 0 for register, 1 for value
	rvswd_write_bit(0); // ??? Seems to have something to do with halting?

    rvswd_stop();

read_end:
    interrupts();
    sleep_us(STOP_WAIT);
    return (parity == parity_read) ? 0 : -1;
}
