#ifndef WS2812_H_
#define WS2812_H_

#include "hardware/pio.h"


/*! Initialization of WS2812B/NeoPIXEL
* \param pio    Defined module for Programmable Input/Output (pio0 or pio1)
* \param pin    Defined GPIO for transmitting data to LED
*/
void ws2812_init(PIO pio, uint8_t pin);


/*! Transmitting Pixel data to bus with WS2812B/NeoPIXELs
* \param r  Pixel value for the red LED       
* \param g  Pixel value for the green LED
* \param b  Pixel value for the blue LED
*/
void put_pixel_rgb(uint8_t r, uint8_t g, uint8_t b);

#endif