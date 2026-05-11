#include "peri/fpga/flash.h"
#include <stdlib.h>


// ======================== DEFINES =========================
#define FLASH_CMD_WRITE_ENABLE      0x06
#define FLASH_CMD_STATUS_REG0       0x05
#define FLASH_CMD_WRITE_DISABLE     0x04
#define FLASH_CMD_MANU              0x90
#define FLASH_CMD_JEDEC             0x9F
#define FLASH_CMD_ERASE_ALL         0xC7
#define FLASH_CMD_ERASE_3SECTOR     0xD8
#define FLASH_CMD_ERASE_4SECTOR     0xDC
#define FLASH_INS_3READ_SLOW        0x03
#define FLASH_INS_3READ_FAST        0x0B
#define FLASH_INS_4READ_SLOW        0x13
#define FLASH_INS_4READ_FAST        0x0C
#define FLASH_INS_3WRITE            0x02
#define FLASH_INS_4WRITE            0x12


// ===================== CALLABLE FUNCS (FLASH DATA) =====================
bool flash_data_init(flash_data_t *data, size_t page_size){
    data->data = malloc(page_size * sizeof(uint8_t));

    data->position_max = page_size;
    data->position = 0;
    data->address = 0;
    
    return true;
}


bool flash_data_write_byte(flash_data_t *data, uint8_t byte){
    if(data->position >= data->position_max){
        return false;
    }
    data->data[data->position++] = byte;
    return true;
}


uint8_t flash_data_read_byte(flash_data_t *data, uint16_t position){
    if(position >= data->position_max){
        return 0;
    }
    return data->data[position];
}


// ===================== INTERNAL FUNCS =====================
bool flash_send_transmission(flash_fpga_t *config, uint8_t *datatx, size_t datatx_len){
    return spi_write_blocking(config->spi->spi_mod, datatx, datatx_len);
}

bool flash_receive_transmission(flash_fpga_t *config, uint8_t *data_rx, size_t datarx_len){
    return spi_read_blocking(config->spi->spi_mod, 0x00, data_rx, datarx_len);
}


bool flash_program_spi_disable(flash_fpga_t *config){
    if(!config->flash_active){
        return false;
    }

    gpio_set_function(config->spi->pin_mosi, GPIO_FUNC_NULL);
    gpio_set_function(config->spi->pin_sclk, GPIO_FUNC_NULL);
    gpio_set_function(config->spi->pin_miso, GPIO_FUNC_NULL);    
    gpio_set_function(config->gpio_csn, GPIO_FUNC_NULL);
    sleep_us(1);
    config->flash_active = false;
    return true;
}


bool flash_program_spi_enable(flash_fpga_t *config){ 
    if(config->flash_active){
        return true;
    }

    configure_spi_module(config->spi, false);
    gpio_init(config->gpio_csn);
    gpio_set_function(config->gpio_csn, GPIO_FUNC_SIO);
    gpio_set_dir(config->gpio_csn, GPIO_OUT);
    gpio_put(config->gpio_csn, true);
    sleep_us(1);

    config->flash_active = true;
    return true;
}


bool flash_program_write_enable(flash_fpga_t *config){
    if(config->is_write_enabled){
        return true;
    }

    uint8_t data0[1] = {FLASH_CMD_WRITE_ENABLE};
    send_data_spi_module(config->spi, config->gpio_csn, data0, sizeof(data0));
    sleep_us(1);

    config->is_write_enabled = true;
    return config->is_write_enabled;
}


bool flash_program_write_disable(flash_fpga_t *config){
    if(!config->is_write_enabled){
        return false;
    }

    uint8_t data0[1] = {FLASH_CMD_WRITE_DISABLE};
    send_data_spi_module(config->spi, config->gpio_csn, data0, sizeof(data0));
    sleep_us(1);

    config->is_write_enabled = false;
    return config->is_write_enabled;
}


// ===================== CALLABLE FUNCS (FLASH DEVICE) =====================
bool fpga_program_init(flash_fpga_t *config){    
    gpio_init(config->gpio_progb);
    gpio_set_dir(config->gpio_progb, GPIO_OUT);
    gpio_put(config->gpio_progb, false);

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

    flash_data_init(&config->flash_data, config->page_size);

    // Prepare SPI module with high-Z mode on SPI pins
    flash_program_spi_disable(config);

    config->init_done = true;
    return config->init_done;
}


bool fpga_program_do(flash_fpga_t *config, bool do_reset){    
    gpio_put(config->gpio_progb, !do_reset);
    
    if(do_reset){
        flash_program_spi_enable(config);
        flash_program_write_disable(config);
    } else {
        flash_program_write_disable(config);
        flash_program_spi_disable(config);
    }        
    return true;
}


bool fpga_program_reset_do(flash_fpga_t *config){
    flash_program_write_disable(config);
    flash_program_spi_disable(config);

    // Reset programming sequence
    gpio_put(config->gpio_progb, false);
    sleep_ms(1);
    gpio_put(config->gpio_progb, true);
    sleep_ms(1);

    if(config->use_done){
        while(!gpio_get(config->gpio_done))
            sleep_us(10);
    }
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


uint16_t flash_get_status_register(flash_fpga_t *config){
    uint8_t data_tx[1] = {FLASH_CMD_STATUS_REG0};
    uint8_t data_rx[1] = {0x00};
    
    flash_program_spi_enable(config);
    flash_program_write_disable(config);

    gpio_put(config->gpio_csn, false);
    flash_send_transmission(config, data_tx, sizeof(data_tx));
    flash_receive_transmission(config, data_rx, sizeof(data_rx));
    gpio_put(config->gpio_csn, true);
    sleep_us(1);

    flash_program_spi_disable(config);
    return data_rx[0];
}


uint16_t flash_get_device_id(flash_fpga_t *config){
    uint8_t data_tx[4] = {FLASH_CMD_MANU};
    uint8_t data_rx[2] = {0x00};

    flash_program_spi_enable(config);
    flash_program_write_disable(config);

    gpio_put(config->gpio_csn, false);
    flash_send_transmission(config, data_tx, sizeof(data_tx));
    flash_receive_transmission(config, data_rx, sizeof(data_rx));
    gpio_put(config->gpio_csn, true);
    sleep_us(1);

    flash_program_spi_disable(config);
    return (data_rx[0] << 8) | data_rx[1];
}


uint16_t flash_get_jedec_id(flash_fpga_t *config){
    uint8_t data_tx[1] = {FLASH_CMD_JEDEC};
    uint8_t data_rx[3] = {0x00};

    flash_program_spi_enable(config);
    flash_program_write_disable(config);

    gpio_put(config->gpio_csn, false);
    flash_send_transmission(config, data_tx, sizeof(data_tx));
    flash_receive_transmission(config, data_rx, sizeof(data_rx));
    gpio_put(config->gpio_csn, true);
    sleep_us(1);

    flash_program_spi_disable(config);
    return (data_rx[1] << 8) | data_rx[2];
}


uint8_t flash_get_manufacturer_id(flash_fpga_t *config){
    uint8_t data_tx[1] = {FLASH_CMD_JEDEC};
    uint8_t data_rx[3] = {0x00};

    flash_program_spi_enable(config);
    flash_program_write_disable(config);

    gpio_put(config->gpio_csn, false);
    flash_send_transmission(config, data_tx, sizeof(data_tx));
    flash_receive_transmission(config, data_rx, sizeof(data_rx));
    gpio_put(config->gpio_csn, true);
    sleep_us(1);

    flash_program_spi_disable(config);
    return data_rx[0];
}


bool flash_write_data(flash_fpga_t *config, uint32_t start_address, uint8_t data[], size_t data_len){
    if(gpio_get(config->gpio_progb) == true){
        return false;
    }
    config->flash_data.position = 0;
    config->flash_data.address = start_address;

    uint8_t data_tx[5] = {0};
    uint8_t data_tx_len = 0;

    if(config->use_32bit) {
        data_tx[0] = FLASH_INS_4WRITE;
        data_tx[1] = (uint8_t)(start_address >> 24);
        data_tx[2] = (uint8_t)(start_address >> 16);
        data_tx[3] = (uint8_t)(start_address >> 8);
        data_tx[4] = (uint8_t)(start_address >> 0);
        data_tx_len = 5;
    } else {
        data_tx[0] = FLASH_INS_3WRITE;
        data_tx[1] = (uint8_t)(start_address >> 16);
        data_tx[2] = (uint8_t)(start_address >> 8);
        data_tx[3] = (uint8_t)(start_address >> 0);
        data_tx_len = 4;
    }

    flash_program_spi_enable(config);
    flash_program_write_enable(config);

    gpio_put(config->gpio_csn, false);
    flash_send_transmission(config, data_tx, data_tx_len);
    flash_send_transmission(config, data, data_len);
    gpio_put(config->gpio_csn, true);

    do {
        sleep_us(1);
    } while (!flash_erasing_is_done(config));
    config->is_write_enabled = false;
    
    return true;
}


bool flash_write_data_from_buffer(flash_fpga_t *config){
    return flash_write_data(config, config->flash_data.address, config->flash_data.data, config->flash_data.position_max);
}


bool flash_read_data(flash_fpga_t *config, uint32_t start_address, uint8_t data_rx[], size_t datarx_len){
    uint8_t data_tx[5] = {0};
    uint8_t data_tx_len = 0;

    if(config->use_32bit) {
        data_tx[0] = FLASH_INS_4READ_SLOW;
        data_tx[1] = (uint8_t)(start_address >> 24);
        data_tx[2] = (uint8_t)(start_address >> 16);
        data_tx[3] = (uint8_t)(start_address >> 8);
        data_tx[4] = (uint8_t)(start_address >> 0);
        data_tx_len = 5;
    } else {
        data_tx[0] = FLASH_INS_3READ_SLOW;
        data_tx[1] = (uint8_t)(start_address >> 16);
        data_tx[2] = (uint8_t)(start_address >> 8);
        data_tx[3] = (uint8_t)(start_address >> 0);
        data_tx_len = 4;
    }

    flash_program_spi_enable(config);
    flash_program_write_disable(config);

    gpio_put(config->gpio_csn, false);
    flash_send_transmission(config, data_tx, sizeof(data_tx));
    flash_receive_transmission(config, data_rx, datarx_len);
    gpio_put(config->gpio_csn, true);

    sleep_us(1);
    return true;
}


bool flash_read_data_from_buffer(flash_fpga_t *config){
    return flash_read_data(config, config->flash_data.address, config->flash_data.data, config->flash_data.position_max);
}


bool flash_erasing_sector_complete(flash_fpga_t *config, uint32_t start_address){
    flash_erasing_sector_start(config, start_address);
    do {
        sleep_us(10);
    } while (!flash_erasing_is_done(config));
    return flash_erasing_stop(config);
}


bool flash_erasing_all_complete(flash_fpga_t *config){
    flash_erasing_all_start(config);
    do {
        sleep_us(10);
    } while (!flash_erasing_is_done(config));
    return flash_erasing_stop(config);
}


bool flash_erasing_all_start(flash_fpga_t *config){
    uint8_t data_tx[1] = {FLASH_CMD_ERASE_ALL};

    flash_program_spi_enable(config);
    flash_program_write_enable(config);

    gpio_put(config->gpio_csn, false);
    flash_send_transmission(config, data_tx, sizeof(data_tx));
    gpio_put(config->gpio_csn, true);
    sleep_us(1);

    return true;
}


bool flash_erasing_sector_start(flash_fpga_t *config, uint32_t start_address){
    uint8_t data_tx[5] = {0};
    uint8_t data_tx_len = 0;

    if(config->use_32bit) {
        data_tx[0] = FLASH_CMD_ERASE_4SECTOR;
        data_tx[1] = (uint8_t)(start_address >> 24);
        data_tx[2] = (uint8_t)(start_address >> 16);
        data_tx[3] = (uint8_t)(start_address >> 8);
        data_tx[4] = (uint8_t)(start_address >> 0);
        data_tx_len = 5;
    } else {
        data_tx[0] = FLASH_CMD_ERASE_3SECTOR;
        data_tx[1] = (uint8_t)(start_address >> 16);
        data_tx[2] = (uint8_t)(start_address >> 8);
        data_tx[3] = (uint8_t)(start_address >> 0);
        data_tx_len = 4;
    }    

    flash_program_spi_enable(config);
    flash_program_write_enable(config);

    gpio_put(config->gpio_csn, false);
    flash_send_transmission(config, data_tx, sizeof(data_tx));
    gpio_put(config->gpio_csn, true);
    sleep_us(1);

    return true;
}


bool flash_erasing_is_done(flash_fpga_t *config){
    uint8_t cmd[1] = {FLASH_CMD_STATUS_REG0};
    uint8_t status[1] = {0x00};

    gpio_put(config->gpio_csn, false);
    flash_send_transmission(config, cmd, sizeof(cmd));
    flash_receive_transmission(config, status, sizeof(status));
    gpio_put(config->gpio_csn, true);

    return !(status[0] & 0x01);
}


bool flash_erasing_stop(flash_fpga_t *config) {
    flash_program_write_disable(config);

    sleep_us(1);
    return flash_program_spi_disable(config);
}
