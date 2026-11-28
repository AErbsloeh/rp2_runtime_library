#include <stdio.h>
#include "pico/stdlib.h"

#include "hal/spi/spi.h"
#include "dac/ad57x4/ad57x4.h"


int main(){
    stdio_init_all();

    static ad57x4_t setting_device = {
        .spi_mod = &DEVICE_SPI_DEFAULT,
        .gpio_num_csn = PICO_DEFAULT_SPI_CSN_PIN,
        .use_gpio_ldac = false,
        .gpio_num_ldac = 0,
        .use_gpio_dclr = false,
        .gpio_num_dclr = 0,
        .range_mode = AD57x4_RANGE_UNI_5V,
        .en_pwr_chnnl = AD57x4_PWR_DAC_All,
        .bitwidth = 16,
        .spi_sdo_disable = true,
        .init_done = false
    };

    // Init of device
    if(ad57x4_init(&setting_device))
        printf("Init of DAC AD57x4 done\n");

    //Main Loop for communication
    uint16_t dac_out = 0;
    while (true){
        ad57x4_update_data(&setting_device, AD57x4_ADR_DAC_ALL, dac_out);
        dac_out += 256;
        sleep_ms(100);
    };
}
