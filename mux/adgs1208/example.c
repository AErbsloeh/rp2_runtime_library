#include <stdio.h>
#include "pico/stdlib.h"

#include "hal/spi/spi.h"
#include "mux/adgs1208/adgs1208.h"


int main(){
    stdio_init_all();

    static adgs1208_t setting_device = {
        .spi_handler = &DEVICE_SPI_DEFAULT,
        .gpio_num_csn = PICO_DEFAULT_SPI_CSN_PIN,
        .num_device_dc = 1,
        .use_rstn_hw = false,
        .gpio_num_rstn = 5,
        .gpio_1_state = false,
        .gpio_2_state = true,
        .gpio_3_state = false,
        .gpio_4_state = true,
        .init_done = false
    };

    // Init of device
    if(adgs1208_init(&setting_device))
        printf("Init of MUX ADGS1208 done\n");

    //Main Loop for communication (0: disable, 1: S1, ..., 8: S8)
    uint8_t pos[1] = {1};

    while (true){
        adgs1208_define_output(&setting_device, pos);
        if(pos[0] >= 8){
            pos[0] = 1;
        } else {
            pos[0]++;
        };
        sleep_ms(250);
    };
}
