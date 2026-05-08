#include "hal/spi/spi.h"
#include "peri/fpga/flash.h"
#include "peri/fpga/fpga_spi.h"


static spi_rp2_t spi_flash_env5 = {
        .spi_mod = spi0,
        .pin_mosi = 3,
        .pin_sclk = 2,
        .pin_miso = 0,
        .fspi_khz = 8000,
        .mode = 0,
        .msb_first = true,
        .init_done = false
};
static flash_fpga_t flash_env5 = {
    .spi = &spi_flash_env5,
    .gpio_csn = 1,
    .gpio_progb = 12,
    .gpio_initb = 255,
    .use_initb = false,
    .gpio_done = 255,
    .use_done = false,
    .flash_active = false,
    .init_done = false,
    .page_size = 256,
};
static spi_rp2_t spi_fpga_env5 = {
    .spi_mod = spi0,
    .pin_mosi = 19,
    .pin_sclk = 18,
    .pin_miso = 16,
    .fspi_khz = 8000,
    .mode = 0,
    .msb_first = true,
    .init_done = false,
};
static fpga_spi_t fpga_env5 = {
    .spi = &spi_fpga_env5,
    .number_bytes = 3,
    .gpio_csn = 17,
    .gpio_rstn = 20,
    .gpio_rdy = 15,
    .init_done = false,
};
