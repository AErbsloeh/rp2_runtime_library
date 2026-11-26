#ifndef PIO_CLK_H
#define PIO_CLK_H


#include "hardware/pio.h"


/*! \brief Function for initialising and starting a PIO function for generating a CLK signal on defined PIN
    \param pio          PIO module
    \param pin          GPIO number for CLK generation
    \param freq         Frequency value of generated CLK signal in Hz
    \return             Boolean if the CLK is running, false if the frequency is 0 or too high for the system clock (half period has to be at least 3 system clock cycles, the PIO program needs 3 cycles more than the passed counter value) or no state machine or program memory is free
*/
bool clk_generation_pio_init(PIO pio, uint pin, uint freq_hz);


#endif

