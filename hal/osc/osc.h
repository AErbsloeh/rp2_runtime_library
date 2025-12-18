#ifndef HAL_XOSC_H_
#define HAL_XOSC_H_


#include "pico/stdlib.h"


/*! Function for redefining the System Clock
* \param new_clock_khz  Integer value with new system clock value
* \return               Boolean if clock is setted and stable
*/
bool set_system_clock(uint32_t new_clock_khz);


#endif
