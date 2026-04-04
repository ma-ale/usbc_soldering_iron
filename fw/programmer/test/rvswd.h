#pragma once

#include <stdint.h>

typedef struct rvswd_handle {
    int swdio;
    int swclk;
} rvswd_handle_t;

typedef enum rvswd_result {
    RVSWD_OK = 0,
    RVSWD_FAIL = 1,
    RVSWD_INVALID_ARGS = 2,
    RVSWD_PARITY_ERROR = 3,
} rvswd_result_t;

rvswd_result_t rvswd_init(rvswd_handle_t* handle);
rvswd_result_t rvswd_reset(rvswd_handle_t* handle);
rvswd_result_t rvswd_write(rvswd_handle_t* handle, uint8_t reg, uint32_t value);
rvswd_result_t rvswd_read(rvswd_handle_t* handle, uint8_t reg, uint32_t* value);
