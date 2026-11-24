#include "src/fpga_spi_shared.h"


bool fpga_spi_shared_is_ready(fpga_spi_shared_t *bus){
    if(bus->flash->flash_active){
        return false;
    }
    return (bus->flash->use_done) ? gpio_get(bus->flash->gpio_done) : true;
}


bool fpga_spi_shared_attach(fpga_spi_shared_t *bus){
    if(bus->attached){
        return true;
    }
    if(bus->flash->spi != bus->logic->spi || !fpga_spi_shared_is_ready(bus)){
        return false;
    }
    sleep_ms(1);

    gpio_init(bus->flash->gpio_csn);
    gpio_set_dir(bus->flash->gpio_csn, GPIO_OUT);
    gpio_put(bus->flash->gpio_csn, true);

    bus->logic->spi->fspi_khz = bus->fspi_khz_logic;
    configure_spi_module(bus->logic->spi, false);
    bus->attached = true;
    return true;
}


void fpga_spi_shared_detach(fpga_spi_shared_t *bus){
    if(!bus->attached){
        return;
    }
    gpio_set_function(bus->logic->spi->pin_mosi, GPIO_FUNC_NULL);
    gpio_set_function(bus->logic->spi->pin_sclk, GPIO_FUNC_NULL);
    gpio_set_function(bus->logic->spi->pin_miso, GPIO_FUNC_NULL);
    gpio_set_function(bus->flash->gpio_csn, GPIO_FUNC_NULL);
    bus->logic->spi->fspi_khz = bus->fspi_khz_flash;
    bus->attached = false;
}


bool fpga_spi_shared_transfer(fpga_spi_shared_t *bus, uint8_t *data_tx, uint8_t *data_rx){
    if(!fpga_spi_shared_attach(bus)){
        return false;
    }
    return fpga_logic_send_data(bus->logic, data_tx, data_rx);
}
