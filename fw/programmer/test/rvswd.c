#include "rvswd.h"
#include <inttypes.h>
#include <stdint.h>
#include <hardware/gpio.h>
#include <pico/time.h>


// adapted from https://github.com/Nicolai-Electronics/esp32-component-rvswd

rvswd_result_t rvswd_init(rvswd_handle_t* handle) {
    //gpio_config_t swio_cfg = {
    //    .pin_bit_mask = BIT64(handle->swdio),
    //    .mode = GPIO_MODE_INPUT_OUTPUT_OD,
    //    .pull_up_en = true,
    //    .pull_down_en = false,
    //    .intr_type = GPIO_INTR_DISABLE,
    //};
    gpio_init(handle->swdio);
    gpio_set_pulls(handle->swdio, true, false);
    gpio_put(handle->swdio, 0);
    gpio_set_dir(handle->swdio, GPIO_IN);

    //gpio_config_t swck_cfg = {
    //    .pin_bit_mask = BIT64(handle->swclk),
    //    .mode = GPIO_MODE_OUTPUT,
    //    .pull_up_en = false,
    //    .pull_down_en = false,
    //    .intr_type = GPIO_INTR_DISABLE,
    //};
    gpio_init(handle->swclk);
    gpio_set_pulls(handle->swclk, false, false);
    gpio_put(handle->swclk, 0);
    gpio_set_dir(handle->swclk, GPIO_OUT);

    return RVSWD_OK;
}

rvswd_result_t rvswd_start(rvswd_handle_t* handle) {
    // Start with both lines high
    // open drain emulation, input for high (pulled up) and out for low (forced)
    gpio_set_dir(handle->swdio, GPIO_IN); // high
    gpio_put(handle->swclk, 1);
    sleep_us(2);

    // Pull data low
    gpio_set_dir(handle->swdio, GPIO_OUT); // low
    gpio_put(handle->swclk, 1);
    sleep_us(1);

    // Pull clock low
    gpio_set_dir(handle->swdio, GPIO_OUT); // low
    gpio_put(handle->swclk, 0);
    sleep_us(1);

    return RVSWD_OK;
}

rvswd_result_t rvswd_stop(rvswd_handle_t* handle) {
    // Pull data low
    gpio_set_dir(handle->swdio, GPIO_OUT);
    sleep_us(1);
    gpio_put(handle->swclk, 1);
    sleep_us(2);

    // Let data float high
    gpio_set_dir(handle->swdio, GPIO_IN);
    sleep_us(1);
    return RVSWD_OK;
}

rvswd_result_t rvswd_reset(rvswd_handle_t* handle) {
    // set data floating high
	gpio_set_dir(handle->swdio, GPIO_IN);
    sleep_us(1);
    // let clock go brrr
    for (uint8_t i = 0; i < 100; i++) {
        gpio_put(handle->swclk, 0);
        sleep_us(1);
        gpio_put(handle->swclk, 1);
        sleep_us(1);
    }
    return rvswd_stop(handle);
}

#define OD_PULL(pin, value) gpio_set_dir((pin), (value) ? GPIO_IN : GPIO_OUT)

void rvswd_write_bit(rvswd_handle_t* handle, bool value) {
    OD_PULL(handle->swdio, value);
    gpio_put(handle->swclk, 0);
    // FIXME: does this need a delay?
    gpio_put(handle->swclk, 1);  // Data is sampled on rising edge of clock
}

bool rvswd_read_bit(rvswd_handle_t* handle) {
	// let data float high
    gpio_set_dir(handle->swdio, GPIO_IN);
    // pulse clock
    gpio_put(handle->swclk, 0);
    gpio_put(handle->swclk, 1);  // Data is output on rising edge of clock
    // FIXME: does this need a delay?
    return gpio_get(handle->swdio);
}

rvswd_result_t rvswd_write(rvswd_handle_t* handle, uint8_t reg, uint32_t value) {
    rvswd_start(handle);

    // ADDR HOST
    bool parity = false;  // This time it's odd parity?
    for (uint8_t position = 0; position < 7; position++) {
        bool bit = (reg >> (6 - position)) & 1;
        rvswd_write_bit(handle, bit);
        if (bit) parity = !parity;
    }

    // Operation: write
    rvswd_write_bit(handle, true);
    parity = !parity;

    // Parity bit (even)
    rvswd_write_bit(handle, parity);

    rvswd_write_bit(handle, 1);
    rvswd_write_bit(handle, 0);
    rvswd_write_bit(handle, 1);
    rvswd_write_bit(handle, 0);
    rvswd_write_bit(handle, 1);

    // Data
    parity = false;  // This time it's even parity?
    for (uint8_t position = 0; position < 32; position++) {
        bool bit = (value >> (31 - position)) & 1;
        rvswd_write_bit(handle, bit);
        if (bit) parity = !parity;
    }

    // Parity bit
    rvswd_write_bit(handle, parity);

    rvswd_write_bit(handle, 1);
    rvswd_write_bit(handle, 0);
    rvswd_write_bit(handle, 1);
    rvswd_write_bit(handle, 1);
    rvswd_write_bit(handle, 1);

    rvswd_stop(handle);

    return RVSWD_OK;
}

rvswd_result_t rvswd_read(rvswd_handle_t* handle, uint8_t reg, uint32_t* value) {
    bool parity;

    rvswd_start(handle);

    // ADDR HOST
    parity = false;
    for (uint8_t position = 0; position < 7; position++) {
        bool bit = (reg >> (6 - position)) & 1;
        rvswd_write_bit(handle, bit);
        if (bit) parity = !parity;
    }

    // Operation: read
    rvswd_write_bit(handle, false);

    // Parity bit (even)
    rvswd_write_bit(handle, parity);

    rvswd_write_bit(handle, 1);
    rvswd_write_bit(handle, 0);
    rvswd_write_bit(handle, 1);
    rvswd_write_bit(handle, 0);
    rvswd_write_bit(handle, 1);

    *value = 0;

    // Data
    parity = false;
    for (uint8_t position = 0; position < 32; position++) {
        bool bit = rvswd_read_bit(handle);
        if (bit) {
            *value |= 1 << (31 - position);
        }
        if (bit) parity = !parity;
    }

    // Parity bit
    bool parity_read = rvswd_read_bit(handle);

    rvswd_write_bit(handle, 1);
    rvswd_write_bit(handle, 0);
    rvswd_write_bit(handle, 1);
    rvswd_write_bit(handle, 1);
    rvswd_write_bit(handle, 1);

    rvswd_stop(handle);

    return (parity == parity_read) ? RVSWD_OK : RVSWD_FAIL;
}
