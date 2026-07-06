#include "peri/fpga/logic.h"


bool fpga_logic_init(fpga_logic_t *config){
    // Init SPI module
    if(!config->spi->init_done){
        configure_spi_module(config->spi, false);
    }

    // Init GPIOs
    gpio_init(config->gpio_csn);
    gpio_set_dir(config->gpio_csn, GPIO_OUT);
    gpio_put(config->gpio_csn, true);

    gpio_init(config->gpio_rstn);
    gpio_set_dir(config->gpio_rstn, GPIO_OUT);
    gpio_put(config->gpio_rstn, false);

    gpio_init(config->gpio_rdy);
    gpio_set_dir(config->gpio_rdy, GPIO_IN);
    gpio_pull_up(config->gpio_rdy);

    config->init_done = true;
    return true;
}


bool fpga_logic_reset_do(fpga_logic_t *config, bool enable){
    gpio_put(config->gpio_rstn, !enable);
    return gpio_get(config->gpio_rstn);
}


bool fpga_logic_reset_cycle(fpga_logic_t *config, uint8_t num_iterations){
    for(uint8_t ite=0; ite < num_iterations; ite++){
        gpio_put(config->gpio_rstn, false);
        sleep_ms(10);
        gpio_put(config->gpio_rstn, true);
        sleep_ms(10);
    }
    return gpio_get(config->gpio_rstn);    
}


bool fpga_logic_send_data(fpga_logic_t *config, uint8_t *data, uint8_t *data_rx){
    gpio_put(config->gpio_csn, false);
    int8_t status = spi_write_read_blocking(config->spi->spi_mod, data, data_rx, config->number_bytes);
    gpio_put(config->gpio_csn, true);
    return status > 0;
}
