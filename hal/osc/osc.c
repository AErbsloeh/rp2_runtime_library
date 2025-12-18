#include "hal/osc/osc.h"
#include "hardware/clocks.h"


bool set_system_clock(uint32_t new_clock_khz){
    set_sys_clock_khz(new_clock_khz, true);
}
