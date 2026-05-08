// ----------------- TEMPLATE CODE TO BUILD AN EXAMPLE -----------------
#include <stdio.h>
#include "hal/led/led.h"
#include "hal/spi/spi.h"
#include "peri/fpga/flash.h"
#include "peri/fpga/fpga_spi.h"
#include "peri/fpga/fpga_config.h"


#define FPGA_EN_POWER_GPIO  23


int main(){   
    /* --- Init Phase --- */
    init_default_led(); 

     // Pin Init for FPGA Power Enable
    gpio_init(FPGA_EN_POWER_GPIO);
    gpio_set_dir(FPGA_EN_POWER_GPIO, GPIO_OUT);
    gpio_put(FPGA_EN_POWER_GPIO, false);

    // SPI
    stdio_init_all();
    sleep_ms(3000);

    // Pre-Phase
    uint8_t num_init = 0;
    if(fpga_program_init(&flash_env5))
        num_init++;

    if(fpga_spi_init(&fpga_env5))
        num_init++;

    if(num_init == 2)
        printf("System initializations done!\n");
    else
        while(1){
            sleep_ms(1000);
        }

    /* --- Reading flash meta information --- */
    uint16_t ret = fpga_flash_get_device_id(&flash_env5);
    printf("\n");
    printf("Device ID: %04x\n", ret);
    sleep_ms(1);
    
    ret = fpga_flash_get_jedec_id(&flash_env5);
    printf("JEDEC ID: %04x\n", ret);
    sleep_ms(1);

    ret = fpga_flash_get_manufacturer_id(&flash_env5);
    printf("Manufacturer ID: %04x\n", ret);
    sleep_ms(1);

    ret = fpga_flash_get_status_register(&flash_env5);
    printf("Status Register: %04x\n", ret);
    sleep_ms(1);


    /* --- Erasing flash content --- */
    printf("\nErasing flash content...\n");
    fpga_flash_erasing_complete(&flash_env5);
    /*fpga_flash_erasing_start(&flash_env5);
    while(!fpga_flash_erasing_is_done(&flash_env5))
        sleep_ms(100);
    fpga_flash_erasing_stop(&flash_env5);*/
    sleep_ms(1);


    /* --- Reading flash content --- */
    uint8_t data_rx[256];
    fpga_flash_read_data(&flash_env5, 0, data_rx, sizeof(data_rx));
    printf("\nRead flash content (%d bytes) before change:\n", sizeof(data_rx));
    for(uint16_t idx=0; idx < 256; idx++){
        printf("%02x ", data_rx[idx]); 
        if(idx % 16 == 0 && idx > 0)
            printf("\n");
    }    

    /* --- Writing flash content --- */
    data_rx[0] = 0xff;
    data_rx[1] = 0x04;
    data_rx[2] = 0xff;
    printf("\nWrite flash content (%d bytes) before change:\n", sizeof(data_rx));
    bool written_state = fpga_flash_write_data(&flash_env5, 0, data_rx, sizeof(data_rx));
    sleep_ms(1);
    printf("Written state: %02x\n", written_state);


    /* --- Reading flash content --- */
    fpga_flash_read_data(&flash_env5, 0, data_rx, sizeof(data_rx));
    printf("\nRead flash content (%d bytes) after change:\n", sizeof(data_rx));
    for(uint16_t idx=0; idx < 256; idx++){
        printf("%02x ", data_rx[idx]); 
        if(idx % 16 == 0 && idx > 0)
            printf("\n");
    }     
    
    /* --- Enable power to FPGA --- */
    printf("\nEnablng FPGA power!\n");
    gpio_put(FPGA_EN_POWER_GPIO, true);
    sleep_ms(1000);    

    /* --- Main Loop --- */
    uint8_t data_rx0[3] = {0};
    uint8_t data_toggle_led[3] = {0x08, 0x00, 0x00};
    uint16_t cnt = 0;
    bool state = false;

    fpga_spi_reset_do(&fpga_env5, state);
    set_state_default_led(true);
    while (true) {  
        toggle_state_default_led();
        sleep_ms(1000);

        /*fpga_spi_send_data(&fpga_env5, data_toggle_led, data_rx0);    
        state = !state;
        cnt++;
        if(cnt >= 200){
            cnt = 0;
            fpga_program_reset_do(&flash_env5);
        }*/
    }
}
