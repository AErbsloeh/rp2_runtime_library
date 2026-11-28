#include <stdio.h>
#include "pico/stdlib.h"

#include "mux/adg1408/adg1408.h"


int main(){
    stdio_init_all();

    static adg1408_t setting_device = {
        .gpio_num_en = 0,
        .gpio_num_a0 = 1,
        .gpio_num_a1 = 2,
        .gpio_num_a2 = 3,
        .init_done = false
    };

    // Init of device
    if(adg1408_init(&setting_device))
        printf("Init of MUX ADG1408 done\n");

    //Main Loop for communication (0 = disable, 1-8: channel)
    uint8_t cnt = 1;

    while (true){
        adg1408_select_chnnl(&setting_device, cnt);
        sleep_ms(1000);
        if(cnt >= 8){
            cnt = 1;
        } else {
            cnt++;
        };
    };
}
