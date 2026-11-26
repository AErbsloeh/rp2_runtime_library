#ifndef HAL_HELPER_H_
#define HAL_HELPER_H_


#include "pico/stdlib.h"


/*!
 * \brief   Get the current runtime in microseconds since system start
 * \return  uint64_t  with current runtime in microseconds
 */
uint64_t get_runtime_us(void);


#endif
