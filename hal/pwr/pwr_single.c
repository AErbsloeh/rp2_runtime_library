#include "hal/pwr/pwr_single.h"
#include <stdio.h>


void gpio_isr_pwr_single_monitor(uint gpio, uint32_t events, power_single_t *config) {
    (void)gpio;
    (void)events;
    gpio_set_irq_enabled(config->pin_pgd, GPIO_IRQ_EDGE_FALL, false);
    gpio_put(config->pin_en, false);
    config->state = false;
}


bool monitor_system_power_single_pgd_start(uint8_t pin_pgd){
    for(uint16_t idx=0; idx < 10000; idx++){
        sleep_us(10);
        if(!gpio_get(pin_pgd)){
            return false;
        }
    }
    return true;
}


bool init_system_power_single(power_single_t *config){
    gpio_init(config->pin_en);
    gpio_set_dir(config->pin_en, GPIO_OUT);
    gpio_put(config->pin_en, false);

    if(config->use_pgd){
        gpio_init(config->pin_pgd);
        gpio_set_dir(config->pin_pgd, GPIO_IN);
        gpio_pull_down(config->pin_pgd);
    }
    config->init_done = true;
    return config->init_done;
}


bool enable_system_power_single(power_single_t *config){
    if(!config->init_done){
        init_system_power_single(config);
    }
    if(config->state){
        return true;
    }

    if(!config->use_pgd){
        sleep_ms(400);
        gpio_put(config->pin_en, true);
        sleep_ms(400);
        config->state = true;
        return true;
    }

    gpio_put(config->pin_en, true);
    absolute_time_t t_end = make_timeout_time_ms(4000);
    while(!gpio_get(config->pin_pgd)){
        if(time_reached(t_end)){
            disable_system_power_single(config);
            return false;
        }
        sleep_ms(1);
    }

    if(!monitor_system_power_single_pgd_start(config->pin_pgd)){
        disable_system_power_single(config);
        return false;
    }

    if(config->use_pgd_isr){
        gpio_set_slew_rate(config->pin_pgd, GPIO_SLEW_RATE_FAST);
        gpio_set_irq_enabled(config->pin_pgd, GPIO_IRQ_EDGE_FALL, true);
    }
    config->state = true;
    return config->state;
}


bool disable_system_power_single(power_single_t *config){
    if(config->use_pgd_isr){
        gpio_set_irq_enabled(config->pin_pgd, GPIO_IRQ_EDGE_FALL, false);
    }

    gpio_put(config->pin_en, false);
    sleep_ms(100);

    config->state = false;
    return config->state;
}
