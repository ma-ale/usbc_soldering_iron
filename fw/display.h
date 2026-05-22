#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <u8g2.h>

#define SSD1306_128X32 1
// #define SSD1312_96X16 1
#define PIN_DISP_RST PA7  // display reset

#ifdef SSD1306_128X32
#define x_off 0
#define y_off 0
#endif

#ifdef SSD1312_96X16
#define x_off 0
#define y_off 8
#endif

u8g2_t* display_init(void);
const char* i16toa(int16_t value);
const char* u16toa(uint16_t value);
int buf_putc(char c);
const char* buf_get(void);
void draw_temp(u8g2_t *u8g2, uint8_t x, uint8_t y, int16_t temp, bool degc);


#endif // _DISPLAY_H
