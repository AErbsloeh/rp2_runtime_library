#include <stdio.h>
#include "pico/stdlib.h"

#include "poti/ad5141/ad5141_spi.h"


int main(){
    static ad5141_spi_rp2_t setting_device = {
        .spi_handler = &DEVICE_SPI_DEFAULT,
        .gpio_csn = PICO_DEFAULT_SPI_CSN_PIN,
        .init_done = false
    };

    // Init of device
    ad5141_spi_init(&setting_device);

    //Main Loop for communication
    uint8_t pos = 0;
    while (true){
        ad5141_spi_define_level(&setting_device, 0, pos);
        pos++;
        sleep_ms(250);
    };
}
