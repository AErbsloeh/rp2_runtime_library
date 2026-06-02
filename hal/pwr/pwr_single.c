#include "hal/pwr/pwr_single.h"
#include <stdio.h>


// ============================== ISR ROUTINES ==============================
void gpio_isr_pwr_single_monitor(uint gpio, uint32_t events, power_single_t *config) {
    disable_system_power_single(config);
}


// ============================== HELP FUNCTIONS ==============================
bool monitor_system_power_single_pgd_start(uint8_t pin_pgd){
    bool power_not_ready = true;

    for(uint16_t idx=0; idx < 10000; idx++){
        sleep_us(10);
        power_not_ready = gpio_get(pin_pgd);
    }
    return !power_not_ready;
}


// ============================== ROUTINES FOR SINGLE POWER SUPPLY ==============================
bool init_system_power_single(power_single_t *config){
    gpio_init(config->pin_en);
    gpio_set_dir(config->pin_en, GPIO_OUT);    
    gpio_put(config->pin_en, false);

    if(config->use_pgd){
        gpio_init(config->pin_pgd);
        gpio_set_dir(config->pin_pgd, GPIO_IN);
        gpio_pull_up(config->pin_pgd);
    }
    config->init_done = true;
    return config->init_done;
}


bool enable_system_power_single(power_single_t *config){
    if(!config->init_done){
        init_system_power_single(config);
    }
    if(config->use_pgd){
        if(gpio_get(config->pin_pgd)){
            return false;
        }
    }
    
    gpio_put(config->pin_en, true);
    sleep_us(10);

    bool state = false;
    if(config->use_pgd){
        state = monitor_system_power_single_pgd_start(config->pin_pgd);
        if(config->use_pgd_isr){
            gpio_set_slew_rate(config->pin_pgd, GPIO_SLEW_RATE_FAST);
            gpio_set_irq_enabled(config->pin_pgd, GPIO_IRQ_EDGE_FALL, true);
        }
        if(!state) {
            disable_system_power_single(config);
        }
    } else {
        sleep_ms(400);
        state = true;
    }
    config->state = state;
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
