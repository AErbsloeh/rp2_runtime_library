#include "mux/adg1408/adg1408.h"
#include "hardware/gpio.h"


bool adg1408_init(adg1408_t *handler){
    gpio_init(handler->gpio_num_en);
    gpio_set_dir(handler->gpio_num_en, GPIO_OUT);
    gpio_put(handler->gpio_num_en, false);

    gpio_init(handler->gpio_num_a0);
    gpio_set_dir(handler->gpio_num_a0, GPIO_OUT);
    gpio_put(handler->gpio_num_a0, false);

    gpio_init(handler->gpio_num_a1);
    gpio_set_dir(handler->gpio_num_a1, GPIO_OUT);
    gpio_put(handler->gpio_num_a1, false);

    gpio_init(handler->gpio_num_a2);
    gpio_set_dir(handler->gpio_num_a2, GPIO_OUT);
    gpio_put(handler->gpio_num_a2, false);

    handler->init_done = true;
    return handler->init_done;
}


void adg1408_select_chnnl(adg1408_t *handler, uint8_t sel_chnnl){ 
    if(!handler->init_done){
        adg1408_init(handler);
    }

    const uint8_t chnnl = sel_chnnl & 0x0F;
    const bool enable = (chnnl >= 1) && (chnnl <= 8);
    const uint8_t address = enable ? (chnnl - 1) : 0;

    gpio_put(handler->gpio_num_en, enable);
    gpio_put(handler->gpio_num_a0, address & 0x01);
    gpio_put(handler->gpio_num_a1, address & 0x02);
    gpio_put(handler->gpio_num_a2, address & 0x04);
}
