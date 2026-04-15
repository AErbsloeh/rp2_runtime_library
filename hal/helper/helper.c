#include "hal/helper/helper.h"


uint64_t get_runtime_ms(void){
    absolute_time_t now = get_absolute_time();
    return to_us_since_boot(now);
}