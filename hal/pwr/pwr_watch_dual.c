#include "hal/pwr/pwr_watch_dual.h"
#include <stdio.h>


// ============================== ISR ROUTINES ==============================
void gpio_isr_pwr_monitor(uint gpio, uint32_t events, power_dual_t *config) {
    /* ADD CODE HERE WITH RIGHT STRUCT HANDLER */  
    disable_system_power(config);
    gpio_put(config->pin_led, true);   
    while(true){
        sleep_ms(1000);      
        printf("Power down! Please check!\n");
    };
}


// ============================== HELP FUNCTIONS ==============================
bool monitor_system_power_pgd_start(uint8_t pin_pgd, bool enable_callback){
    bool power_not_ready = true;

    for(uint16_t idx=0; idx < 10000; idx++){
        sleep_us(10);
        power_not_ready = gpio_get(pin_pgd);
    }

    if(!power_not_ready){
        sleep_ms(1000);
        gpio_set_irq_enabled(pin_pgd, GPIO_IRQ_EDGE_FALL, true);
    }
    return !power_not_ready;
}


// ============================== ROUTINES FOR DUAL POWER SUPPLY ==============================
bool init_system_power(power_dual_t *config){
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
        gpio_set_irq_enabled(config->pin_pgd, GPIO_IRQ_EDGE_FALL, true);
    }

    config->init_done = true;
    return config->init_done;
}


bool enable_system_power(power_dual_t *config){
    if(!config->init_done){
        init_system_power(config);
    }
    
    sleep_ms(500);
    gpio_put(config->pin_en_reg, true);
    sleep_ms(500);
    gpio_put(config->pin_en_ldo, true);

    bool state = false;
    bool state_led = false;
    if(config->use_pgd){
        state = monitor_system_power_pgd_start(config->pin_pgd, false);
        
        if(!state) {
            disable_system_power(config);
        }
        while(!state){
            gpio_put(config->pin_led, state_led);
            state_led = !state_led;
            sleep_ms(1000);
        }
    }
    config->state = state;
    return config->state;
}


bool disable_system_power(power_dual_t *config){
    gpio_put(config->pin_en_ldo, false);
    sleep_ms(10);
    gpio_put(config->pin_en_reg, false);
    sleep_ms(10);

    config->state = false;
    return config->state;
}
