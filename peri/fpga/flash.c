#include "peri/fpga/flash.h"

/* TODO
- Implement buffer with page size
- What happens with INIT_B if not used?
- Implement RPC for flashing
*/
// ======================== DEFINES =========================
#define FLASH_CMD_WRITE_ENABLE      0x06
#define FLASH_CMD_STATUS_REG0       0x05
#define FLASH_CMD_WRITE_DISABLE     0x04
#define FLASH_CMD_MANU              0x90
#define FLASH_CMD_JEDEC             0x9F
#define FLASH_CMD_ERASE_ALL         0xC7
#define FLASH_INS_READ              0x03
#define FLASH_INS_WRITE             0x02


// ===================== INTERNAL FUNCS =====================
bool fpga_send_data(flash_fpga_t *config, uint8_t *datatx, size_t datatx_len){
    return spi_write_blocking(config->spi->spi_mod, datatx, datatx_len);
}

bool fpga_read_data(flash_fpga_t *config, uint8_t *data_rx, size_t datarx_len){
    return spi_read_blocking(config->spi->spi_mod, 0x00, data_rx, datarx_len);
}


bool fpga_program_disable_spi(flash_fpga_t *config){
    gpio_set_function(config->spi->pin_mosi, GPIO_FUNC_NULL);
    gpio_set_function(config->spi->pin_sclk, GPIO_FUNC_NULL);
    gpio_set_function(config->spi->pin_miso, GPIO_FUNC_NULL);    
    gpio_set_function(config->gpio_csn, GPIO_FUNC_NULL);
    sleep_ms(1);
    gpio_put(config->gpio_progb, true);
    config->flash_active = false;
    return true;
}


bool fpga_program_enable_spi(flash_fpga_t *config){ 
    config->flash_active = true;
    gpio_put(config->gpio_progb, false);
    sleep_ms(1);

    configure_spi_module(config->spi, false);
    gpio_init(config->gpio_csn);
    gpio_set_function(config->gpio_csn, GPIO_FUNC_SIO);
    gpio_set_dir(config->gpio_csn, GPIO_OUT);
    gpio_put(config->gpio_csn, true);
    return true;
}


bool flash_wait_ready(flash_fpga_t *config) {
    do {
        gpio_put(config->gpio_csn, false);
        fpga_send_data(config, cmd, sizeof(cmd));
        fpga_read_data(config, status, sizeof(status));
        gpio_put(config->gpio_csn, true);
    } while (status[0] & 0x01);

    return true;
}

// ===================== CALLABLE FUNCS =====================
bool fpga_program_init(flash_fpga_t *config){    
    gpio_init(config->gpio_progb);
    gpio_set_dir(config->gpio_progb, GPIO_OUT);
    gpio_put(config->gpio_progb, true);

    if(config->use_initb){
        gpio_init(config->gpio_initb);
        gpio_set_dir(config->gpio_initb, GPIO_OUT);
        gpio_put(config->gpio_initb, true);
        gpio_pull_up(config->gpio_initb);
    }
    if(config->use_done){
        gpio_init(config->gpio_done);
        gpio_set_dir(config->gpio_done, GPIO_IN);
        gpio_pull_up(config->gpio_done);
    }

    // Prepare SPI module with high-Z mode on SPI pins
    fpga_program_disable_spi(config);

    config->flash_active = false;
    config->init_done = true;
    return config->init_done;
}


bool fpga_program_reset_do(flash_fpga_t *config){
    // Reset programming sequence
    gpio_put(config->gpio_progb, false);
    sleep_ms(10);
    gpio_put(config->gpio_progb, true);
    sleep_ms(10);

    if(config->use_done){
        while(!gpio_get(config->gpio_done))
            sleep_us(10);
    }
    config->flash_active = false;
    return true;
}


bool fpga_program_check_done(flash_fpga_t *config){
    if(!config->init_done){
        return false;
    }
    
    if(config->use_done){
        const uint16_t timeout_ms = 20000;
        uint16_t elapsed = 0;

        while (!gpio_get(config->gpio_done)) {
            sleep_us(20);
            elapsed++;
            if (elapsed > timeout_ms) {
                return false;
            }
        }
        return true;
    } else {
        sleep_ms(2000);
        return true;
    }
}


uint16_t fpga_flash_get_status_register(flash_fpga_t *config){
    if(!config->flash_active){
        return false;
    }
    fpga_program_enable_spi(config);
    sleep_us(10);

    uint8_t data_tx[1] = {FLASH_CMD_STATUS_REG0};
    uint8_t data_rx[1] = {0x00};
    gpio_put(config->gpio_csn, false);
    fpga_send_data(config, data_tx, sizeof(data_tx));
    fpga_read_data(config, data_rx, sizeof(data_rx));
    gpio_put(config->gpio_csn, true);
    sleep_us(10);

    fpga_program_disable_spi(config);
    return data_rx[0];
}


uint16_t fpga_flash_get_device_id(flash_fpga_t *config){
    fpga_program_enable_spi(config);
    sleep_us(10);

    uint8_t data_tx[4] = {0x00};
    data_tx[0] = FLASH_CMD_MANU;

    uint8_t data_rx[2] = {0x00};

    gpio_put(config->gpio_csn, false);
    fpga_send_data(config, data_tx, sizeof(data_tx));
    fpga_read_data(config, data_rx, sizeof(data_rx));
    gpio_put(config->gpio_csn, true);
    sleep_us(10);

    fpga_program_disable_spi(config);
    return (data_rx[0] << 8) | data_rx[1];
}


uint16_t fpga_flash_get_jedec_id(flash_fpga_t *config){
    fpga_program_enable_spi(config);
    sleep_us(10);

    uint8_t data_tx[1] = {FLASH_CMD_JEDEC};
    uint8_t data_rx[3] = {0x00};

    gpio_put(config->gpio_csn, false);
    fpga_send_data(config, data_tx, sizeof(data_tx));
    fpga_read_data(config, data_rx, sizeof(data_rx));
    gpio_put(config->gpio_csn, true);
    sleep_us(10);

    fpga_program_disable_spi(config);
    return (data_rx[1] << 8) | data_rx[2];
}


uint8_t fpga_flash_get_manufacturer_id(flash_fpga_t *config){
    fpga_program_enable_spi(config);
    sleep_us(10);

    uint8_t data_tx[1] = {FLASH_CMD_JEDEC};
    uint8_t data_rx[3] = {0x00};

    gpio_put(config->gpio_csn, false);
    fpga_send_data(config, data_tx, sizeof(data_tx));
    fpga_read_data(config, data_rx, sizeof(data_rx));
    gpio_put(config->gpio_csn, true);
    sleep_us(10);

    fpga_program_disable_spi(config);
    return data_rx[0];
}


bool fpga_flash_write_data(flash_fpga_t *config, uint32_t start_adress, uint8_t data[], size_t data_len){
    fpga_program_enable_spi(config);
    sleep_us(10);

    uint8_t data0[1] = {FLASH_CMD_WRITE_ENABLE};
    send_data_spi_module(config->spi, config->gpio_csn, data0, sizeof(data0));
    sleep_us(10);

    uint8_t data_tx[4] = {0x00};
    data_tx[0] = FLASH_INS_WRITE;
    data_tx[1] = (start_adress >> 16);
    data_tx[2] = (start_adress >> 8);
    data_tx[3] = (start_adress >> 0);

    gpio_put(config->gpio_csn, false);
    fpga_send_data(config, data_tx, sizeof(data_tx));
    fpga_send_data(config, data, data_len);
    gpio_put(config->gpio_csn, true);

    sleep_us(10);
    data0[0] = FLASH_CMD_WRITE_DISABLE;
    send_data_spi_module(config->spi, config->gpio_csn, data0, sizeof(data0));

    sleep_us(10);
    bool state = flash_wait_ready(config);
    fpga_program_disable_spi(config);
    return state;
}


bool fpga_flash_read_data(flash_fpga_t *config, uint32_t start_adress, uint8_t data_rx[], size_t datarx_len){
    fpga_program_enable_spi(config);
    sleep_ms(1);

    uint8_t data_tx[4] = {0x00};
    data_tx[0] = FLASH_INS_READ;
    data_tx[1] = (start_adress >> 16);
    data_tx[2] = (start_adress >> 8);
    data_tx[3] = (start_adress >> 0);

    gpio_put(config->gpio_csn, false);
    fpga_send_data(config, data_tx, sizeof(data_tx));
    fpga_read_data(config, data_rx, datarx_len);
    gpio_put(config->gpio_csn, true);

    sleep_us(10);
    return fpga_program_disable_spi(config);
}


bool fpga_flash_erasing_complete(flash_fpga_t *config){
    fpga_program_enable_spi(config);
    sleep_us(10);

    uint8_t data0[1] = {FLASH_CMD_WRITE_ENABLE};
    send_data_spi_module(config->spi, config->gpio_csn, data0, sizeof(data0));
    sleep_us(10);

    uint8_t data_tx[1] = {0x00};
    data_tx[0] = FLASH_CMD_ERASE_ALL;

    gpio_put(config->gpio_csn, false);
    fpga_send_data(config, data_tx, sizeof(data_tx));
    gpio_put(config->gpio_csn, true);
    sleep_us(10);

    flash_wait_ready(config);

    sleep_us(10);
    data0[0] = FLASH_CMD_WRITE_DISABLE;
    send_data_spi_module(config->spi, config->gpio_csn, data0, sizeof(data0));

    sleep_us(10);
    return fpga_program_disable_spi(config);
}


bool fpga_flash_erasing_start(flash_fpga_t *config){
    fpga_program_enable_spi(config);
    sleep_us(10);

    uint8_t data0[1] = {FLASH_CMD_WRITE_ENABLE};
    send_data_spi_module(config->spi, config->gpio_csn, data0, sizeof(data0));
    sleep_us(10);

    uint8_t data_tx[1] = {0x00};
    data_tx[0] = FLASH_CMD_ERASE_ALL;

    gpio_put(config->gpio_csn, false);
    fpga_send_data(config, data_tx, sizeof(data_tx));
    gpio_put(config->gpio_csn, true);
    sleep_us(10);

    return true;
}


bool fpga_flash_erasing_is_done(flash_fpga_t *config){
    uint8_t cmd[1] = {FLASH_CMD_STATUS_REG0};
    uint8_t status[1] = {0x00};

    gpio_put(config->gpio_csn, false);
    fpga_send_data(config, cmd, sizeof(cmd));
    fpga_read_data(config, status, sizeof(status));
    gpio_put(config->gpio_csn, true);

    return !(status[0] & 0x01);
}


bool fpga_flash_erasing_stop(flash_fpga_t *config) {
    uint8_t data0[1] = {FLASH_CMD_WRITE_DISABLE};
    send_data_spi_module(config->spi, config->gpio_csn, data0, sizeof(data0));

    sleep_us(10);
    return fpga_program_disable_spi(config);
}


