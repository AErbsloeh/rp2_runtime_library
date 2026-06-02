#include "hal/pwr/pwr_dual.h"
#include <stdio.h>


// ============================== ISR ROUTINES ==============================
void gpio_isr_pwr_dual_monitor(uint gpio, uint32_t events, power_dual_t *config) { 
    disable_system_power_dual(config);
}


// ============================== HELP FUNCTIONS ==============================
bool monitor_system_power_dual_pgd_start(uint8_t pin_pgd){
    bool power_not_ready = true;

    for(uint16_t idx=0; idx < 10000; idx++){
        sleep_us(10);
        power_not_ready = !gpio_get(pin_pgd);
    }
    return !power_not_ready;
}


// ============================== ROUTINES FOR DUAL POWER SUPPLY ==============================
bool init_system_power_dual(power_dual_t *config){
    gpio_init(config->pin_en_reg);
    gpio_set_dir(config->pin_en_reg, GPIO_OUT);    
    gpio_put(config->pin_en_reg, false);

    gpio_init(config->pin_en_ldo);
    gpio_set_dir(config->pin_en_ldo, GPIO_OUT);    
    gpio_put(config->pin_en_ldo, false);

    if(config->use_pgd){
        gpio_init(config->pin_pgd);
        gpio_set_dir(config->pin_pgd, GPIO_IN);
        gpio_pull_up(config->pin_pgd);
    }
    config->init_done = true;
    return config->init_done;
}


bool enable_system_power_dual(power_dual_t *config){
    if(!config->init_done){
        init_system_power_dual(config);
    }
    
    gpio_put(config->pin_en_reg, true);
    sleep_ms(400);
    gpio_put(config->pin_en_ldo, true);
    sleep_ms(50);

    bool state = false;
    if(config->use_pgd){
        if(gpio_get(config->pin_pgd)){
            state = false;
        } else {
            state = monitor_system_power_dual_pgd_start(config->pin_pgd);
            if(config->use_pgd_isr){
                gpio_set_slew_rate(config->pin_pgd, GPIO_SLEW_RATE_FAST);
                gpio_set_irq_enabled(config->pin_pgd, GPIO_IRQ_EDGE_FALL, true);
            }
        }
        if(!state) {
            disable_system_power_dual(config);
        }
    } else {
        sleep_ms(350);
        state = true;
    }
    config->state = state;
    return config->state;
}


bool disable_system_power_dual(power_dual_t *config){
    if(config->use_pgd_isr){
        gpio_set_irq_enabled(config->pin_pgd, GPIO_IRQ_EDGE_FALL, false);
    }

    gpio_put(config->pin_en_ldo, false);
    sleep_ms(100);
    gpio_put(config->pin_en_reg, false);
    sleep_ms(100);

    config->state = false;
    return config->state;
}
