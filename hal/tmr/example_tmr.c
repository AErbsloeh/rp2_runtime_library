#include "hal/tmr/tmr.h"


bool tmr_irq_routine_example(repeating_timer_t *rt){
    return true;
};


int tmr_example_main(void){
    static repeating_timer_t tmr_example;
    static tmr_repeat_irq_t tmr0_example = {
        .timer = &tmr_example,
        .irq_number = 0,
        .period_us = -250000,
        .alarm_done = false,
        .enable_state = false,
        .init_done = false,
        .func_irq = tmr_irq_routine_example
    };

    if(!init_timer_irq(&tmr0_example)){
        return 1;
    }
    enable_repeat_timer_irq(&tmr0_example);
    sleep_ms(1000);
    disable_repeat_timer_irq(&tmr0_example);
    return 0;
}
