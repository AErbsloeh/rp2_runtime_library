#include "hal/tmr/tmr.h"


bool init_timer_irq(tmr_repeat_irq_t* handler){
    if(handler == NULL){
        return false;
    }
    handler->alarm_done = true;
    handler->init_done = true;
    return handler->init_done;
};


bool enable_repeat_timer_irq(tmr_repeat_irq_t* handler){
    if((handler == NULL) || (handler->timer == NULL) || (handler->func_irq == NULL)){
        return false;
    }
    if(!handler->init_done){
        init_timer_irq(handler);
    }

    handler->enable_state = add_repeating_timer_us(handler->period_us, handler->func_irq, NULL, handler->timer);
    return handler->enable_state;
};


bool disable_repeat_timer_irq(tmr_repeat_irq_t* handler){
    if(handler == NULL){
        return false;
    }
    if(!handler->init_done){
        init_timer_irq(handler);
    }

    if(handler->timer != NULL){
        cancel_repeating_timer(handler->timer);
    }
    if(handler->alarm_id > 0){
        cancel_alarm(handler->alarm_id);
        handler->alarm_id = 0;
    }
    handler->enable_state = false;
    return true;
};


bool activate_oneshot_timer_irq(tmr_repeat_irq_t* handler){
    if((handler == NULL) || (handler->func_alarm == NULL)){
        return false;
    }
    if(!handler->init_done){
        init_timer_irq(handler);
    }

    const uint64_t delay_us = (uint64_t)((handler->period_us < 0) ? -handler->period_us : handler->period_us);
    const alarm_id_t id = add_alarm_in_us(delay_us, handler->func_alarm, handler, true);
    handler->alarm_id = (id > 0) ? id : 0;
    handler->enable_state = (id >= 0);
    return handler->enable_state;
};
