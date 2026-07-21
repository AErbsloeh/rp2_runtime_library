#ifndef PIO_CLK_H
#define PIO_CLK_H


#include "hardware/pio.h"


/*! \brief Function for initialising and starting a PIO function for generating a CLK signal on defined PIN
    \param pio          PIO module
    \param pin          GPIO number for CLK generation
    \param freq         Frequency value of generated CLK signal in Hz
*/
bool clk_generation_pio_init(PIO pio, uint pin, uint freq_hz);


#endif

