#include <stdio.h>
#include "pico/stdlib.h"

#include "hal/spi/spi.h"
#include "adc/ads8881/ads8881.h"


int main(){
    stdio_init_all();

    static ads8881_t setting_device = {
        .spi_handler = &DEVICE_SPI_DEFAULT,
        .gpio_num_din = 1,
        .gpio_num_cnv = 0,
        .adc_mode = ADS8881_THREE_WIRE_W_BUSY_IND,
        .invert_out = false,
        .init_done = false
    };

    // Init of device
    if(ads8881_init(&setting_device))
        printf("Init of ADC ADS8881 done\n");

    //Main Loop for communication
    while (true){
        uint32_t data = ads8881_rqst_data(&setting_device);
        printf("ADC data: %lu\n", (unsigned long)data);
        sleep_ms(250);
    };
}
