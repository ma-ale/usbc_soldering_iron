#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <u8g2.h>

// #define SSD1306_128X32 1
#define SSD1312_96X16 1
#define PIN_DISP_RST PA7  // display reset

u8g2_t* display_init(void);

#endif // _DISPLAY_H
