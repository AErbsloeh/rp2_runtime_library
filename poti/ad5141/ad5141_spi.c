#include "poti/ad5141/ad5141_spi.h"
#include <stdio.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"


void ad5141_spi_reset_handler_params(ad5141_spi_rp2_t *gpio_csn)
{
    gpio_csn->init_done = false;
}


bool ad5141_spi_reset_software(ad5141_spi_rp2_t *gpio_csn)
{
    if(!gpio_csn->spi_handler->init_done){
        configure_spi_module(gpio_csn->spi_handler, false);
    } 
    ad5141_spi_reset_handler_params(gpio_csn);

    uint8_t  buffer[2] = {0};
    buffer[0] = 0xB0;
    buffer[1] = 0x00;

    return send_data_spi_module(gpio_csn->spi_handler, gpio_csn->gpio_csn, buffer, 2) == 2;
}


bool ad5141_spi_control_shutdown(ad5141_spi_rp2_t *gpio_csn, bool enable_rdac0, bool enable_rdac1)
{
    if(!gpio_csn->spi_handler->init_done){
        configure_spi_module(gpio_csn->spi_handler, false);
    } 

    bool done = true;
    if(enable_rdac0 == enable_rdac1){
        uint8_t buffer[2] = {0xC8, (enable_rdac0) ? 0x00 : 0x01};
        done = (send_data_spi_module(gpio_csn->spi_handler, gpio_csn->gpio_csn, buffer, 2) == 2);
    } else {
        uint8_t buffer_rdac0[2] = {0xC0, (enable_rdac0) ? 0x00 : 0x01};
        uint8_t buffer_rdac1[2] = {0xC1, (enable_rdac1) ? 0x00 : 0x01};
        done = (send_data_spi_module(gpio_csn->spi_handler, gpio_csn->gpio_csn, buffer_rdac0, 2) == 2);
        done = (send_data_spi_module(gpio_csn->spi_handler, gpio_csn->gpio_csn, buffer_rdac1, 2) == 2) && done;
    }
    return done;
}


bool ad5141_spi_init(ad5141_spi_rp2_t *gpio_csn)
{
    if(!gpio_csn->spi_handler->init_done){
        configure_spi_module(gpio_csn->spi_handler, false);
    } 
	
	gpio_init(gpio_csn->gpio_csn);
    gpio_set_dir(gpio_csn->gpio_csn, GPIO_OUT);
    gpio_set_drive_strength(gpio_csn->gpio_csn, GPIO_DRIVE_STRENGTH_2MA);
    gpio_put(gpio_csn->gpio_csn, true);

    ad5141_spi_reset_software(gpio_csn);
    ad5141_spi_control_shutdown(gpio_csn, true, true);    

    gpio_csn->init_done = true;
    return true;
}


bool ad5141_spi_define_level(ad5141_spi_rp2_t *gpio_csn, uint8_t rdac_sel, uint8_t pot_position){
    if(gpio_csn->init_done){
        uint8_t  buffer[2] = {0};
        buffer[0] = 0x10 | (rdac_sel & 0x0F);
        buffer[1] = pot_position;
        return send_data_spi_module(gpio_csn->spi_handler, gpio_csn->gpio_csn, buffer, 2) == 2;
    } else {
        return false;
    } 
}
