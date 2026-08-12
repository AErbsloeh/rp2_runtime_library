#include <stdio.h>

#include "adc/ads1299/ads1299.h"
#include "hal/spi/spi.h"
#include "hal/led/led.h"


int main(){
    stdio_init_all();
    sleep_ms(1000);

    set_gpio_default_led(2);
    init_default_led();

    // --- Init of the SPI ---
    static spi_rp2_t spi0_inst = {
        .pin_mosi = 19,
        .pin_sclk = 18,
        .pin_miso = 16,
        .spi_mod = spi0,
        .fspi_khz = 1000,
        .mode = 1,          // ADS1299 uses SPI mode 1 (CPOL=0, CPHA=1)
        .msb_first = true,
        .init_done = false,
    };
    configure_spi_module(&spi0_inst, false);
    toggle_state_default_led();

    // --- Init of the ADS1299 ---
    static ads1299_t sens = {
        .spi = &spi0_inst,
        .gpio_cs = 17,
        .gpio_drdy = 20,
        .gpio_reset = 21,
        .gpio_start = 255,
        .num_channels = 8,
        .gain = ADS1299_GAIN_24,
        .data_rate = ADS1299_RATE_250SPS,
        .enable_bias = true,
        .use_internal_reference = true,
        .rdatac_mode = false,
        .init_done = false
    };

    set_state_default_led(true);
    bool init_done = ads1299_init(&sens);
    if(init_done){
        printf("Init ADS1299 sensor done\n");
    } else {
        printf("Init failed");
    }

    if(init_done){
        ads1299_start(&sens);
    }

    uint32_t status = 0;
    int32_t data[8] = {0};

    while (true) {
        sleep_ms(2000);
        if(init_done){
            ads1299_wait_for_data_ready(&sens, 10000);
            ads1299_read_data_all(&sens, &status, data);

            printf("===========Data =======\n");
            printf("Status: 0x%06lX\n", (unsigned long)status);
            for (uint8_t i = 0; i < sens.num_channels; i++){
                printf("CH%d: %ld\n", i, (long)data[i]);
            }
            toggle_state_default_led();
        };
    };
}