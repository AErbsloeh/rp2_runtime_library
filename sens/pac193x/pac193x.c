#include "sens/pac193x/pac193x.h"
#include <stdio.h>

// More details in datasheet (Table 5-1, p.26)
#define PAC193X_I2C_ADDR_START 0x10
#define PAC193X_I2C_ADDR_END   0x1F

#define PAC193X_REG_REFRESH     0x00        // 0 bytes
#define PAC193X_REG_CONTROL     0x01        // 1 byte
#define PAC193X_REG_ACC_CNT     0x02        // 3 bytes
#define PAC193X_REG_VPOWER_ACC  0x03        // 6 bytes
#define PAC193X_REG_VBUS        0x07        // 2 bytes
#define PAC193X_REG_VSENSE      0x0B        // 2 bytes
#define PAC193X_REG_VBUS_AVG    0x0F        // 2 bytes
#define PAC193X_REG_VSENSE_AVG  0x13        // 2 bytes
#define PAC193X_REG_VPOWER      0x17        // 4 bytes
#define PAC193X_REG_ENABLE      0x1C        // 1 byte
#define PAC193X_REG_POLARITY    0x1D        // 1 byte
#define PAC193X_REG_PID         0xFD        // 1 bytes
#define PAC193X_REG_MID         0xFE        // 1 bytes
#define PAC193X_REG_RID         0xFF        // 1 bytes


// ======================= INTERNAL FUNCS =======================
bool pac193x_i2c_write(pac193x_t *config, uint8_t *data, size_t len){
    return construct_i2c_write_data(config->i2c, config->adr, data, len);
}


bool pac193x_i2c_read(pac193x_t *config, uint8_t *data_tx, size_t len_tx, uint8_t *data_rx, size_t len_rx){
    return construct_i2c_read_data(config->i2c, config->adr, data_tx, len_tx, data_rx, len_rx);
}


bool pac193x_send_refresh(pac193x_t *config){  
    uint8_t cmd[1] = {0x00};
    cmd[0] = PAC193X_REG_REFRESH;

    return pac193x_i2c_write(config, cmd, sizeof(cmd));
}


// ======================= CALLABLE FUNCS =======================
uint8_t pac193x_get_i2c_address(uint32_t resistor_value){
    if (resistor_value <= 250)
        return PAC193X_I2C_ADDR_START + 0x00;    // GND (0 Ohm) -> Default
    else if (resistor_value <= 652)
        return PAC193X_I2C_ADDR_START + 0x01;    // 499
    else if (resistor_value <= 1038)
        return PAC193X_I2C_ADDR_START + 0x02;    // 806
    else if (resistor_value <= 1660)
        return PAC193X_I2C_ADDR_START + 0x03;    // 1270
    else if (resistor_value <= 2645)
        return PAC193X_I2C_ADDR_START + 0x04;    // 2050
    else if (resistor_value <= 4235)
        return PAC193X_I2C_ADDR_START + 0x05;    // 3240
    else if (resistor_value <= 6840)
        return PAC193X_I2C_ADDR_START + 0x06;    // 5230
    else if (resistor_value <= 10875)
        return PAC193X_I2C_ADDR_START + 0x07;    // 8450
    else if (resistor_value <= 17400)
        return PAC193X_I2C_ADDR_START + 0x08;    // 13300
    else if (resistor_value <= 27750)
        return PAC193X_I2C_ADDR_START + 0x09;    // 21500
    else if (resistor_value <= 44450)
        return PAC193X_I2C_ADDR_START + 0x0A;    // 34000
    else if (resistor_value <= 71800)
        return PAC193X_I2C_ADDR_START + 0x0B;    // 54900
    else if (resistor_value <= 114350)
        return PAC193X_I2C_ADDR_START + 0x0C;    // 88700
    else if (resistor_value <= 183000)
        return PAC193X_I2C_ADDR_START + 0x0D;    // 140000
    else
        return PAC193X_I2C_ADDR_END;    // VDD (high resistance)
}


bool pac193x_check_product_id(pac193x_t *config){
    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC193X_REG_PID;

    uint8_t data_rx[1] = {0x00};
    if(pac193x_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        if(config->num_channels == 1)       return data_rx[0] == 0x58;
        else if(config->num_channels == 2)  return data_rx[0] == 0x59;
        else if(config->num_channels == 3)  return data_rx[0] == 0x5A;
        else if(config->num_channels == 4)  return data_rx[0] == 0x5B;
        else return false;
    }
    return false;
}


bool pac193x_check_manufacturer_id(pac193x_t *config){
    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC193X_REG_MID;

    uint8_t data_rx[1] = {0x00};
    if(pac193x_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        return data_rx[0] == 0x5D;
    }
    return false;
}


bool pac193x_check_revision_id(pac193x_t *config){
    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC193X_REG_RID;

    uint8_t data_rx[1] = {0x00};
    if(pac193x_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        return data_rx[0] == 0x03;
    }
    return false;
}


uint8_t pac193x_get_number_of_channels(pac193x_t *config){
    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC193X_REG_PID;

    uint8_t data_rx[1] = {0x00};
    if(pac193x_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        if(data_rx[0] == 0x58)       return 1;
        else if(data_rx[0] == 0x59)  return 2;
        else if(data_rx[0] == 0x5A)  return 3;
        else if(data_rx[0] == 0x5B)  return 4;
        else return 0;
    }
    return 0;
}


bool pac193x_set_single_shot_mode(pac193x_t *config, bool enable_single_shot_mode){
    config->enable_single_shot_mode = enable_single_shot_mode;
    return pac193x_set_sampling_rate(config, config->sample_rate);
}


bool pac193x_set_sleep_mode(pac193x_t *config, bool enable_sleep_mode){
    config->enable_sleep_mode = enable_sleep_mode;
    return pac193x_set_sampling_rate(config, config->sample_rate);
}


bool pac193x_set_sampling_rate(pac193x_t *config, uint8_t sample_rate){
    if(sample_rate > 3) {
        return false;
    }
    
    config->sample_rate = sample_rate;
    uint8_t data[2] = {0x00};
    data[0] = PAC193X_REG_CONTROL;
    data[1] = ((config->sample_rate & 0x03) << 6) | 
        ((config->enable_sleep_mode) ? 0x20 : 0x00) | 
        ((config->enable_single_shot_mode) ? 0x10 : 0x00) | 
        0x0A; // Enalbing Alert Pin as OVF Alert

    pac193x_i2c_write(config, data, sizeof(data));
    return pac193x_send_refresh(config);
}


bool pac193x_enable_all_channels(pac193x_t *config, bool enable_channels){
    config->enable_channels = enable_channels;
    uint8_t data[2] = {0x00};
    data[0] = PAC193X_REG_ENABLE;
    data[1] = (config->enable_channels) ? 0x00 : 0x0F;

    pac193x_i2c_write(config, data, sizeof(data));
    return pac193x_send_refresh(config);
}


bool pac193x_set_polarity(pac193x_t *config, bool use_bipolar_current, bool use_bipolar_voltage){
    config->enable_bipolar_voltage = use_bipolar_voltage;
    config->enable_bipolar_current = use_bipolar_current;

    uint8_t data[2] = {0x00};
    data[0] = PAC193X_REG_POLARITY;
    data[1] = ((config->enable_bipolar_voltage) ? 0x0F : 0x00) | 
        ((config->enable_bipolar_current) ? 0xF0 : 0x00);

    pac193x_i2c_write(config, data, sizeof(data));
    return pac193x_send_refresh(config);
}


bool pac193x_init(pac193x_t *config){
    if(config->gpio_pwrdwn != 255){
        gpio_init(config->gpio_pwrdwn);
        gpio_set_dir(config->gpio_pwrdwn, GPIO_OUT);
        gpio_pull_up(config->gpio_pwrdwn);
        gpio_put(config->gpio_pwrdwn, true);
    }
    if(config->gpio_alert != 255){
        gpio_init(config->gpio_alert);
        gpio_set_dir(config->gpio_alert, GPIO_OUT);
        gpio_pull_up(config->gpio_alert);
        gpio_put(config->gpio_alert, false);
    }

    if(!config->i2c->init_done){
        init_i2c_module(config->i2c);
    }

    config->init_done = false; 
    if(config->adr < PAC193X_I2C_ADDR_START || config->adr > PAC193X_I2C_ADDR_END){
        return false;
    }
    if(!check_i2c_bus_for_device_specific(config->i2c, config->adr)){
        return false;
    }
    if(config->num_channels == 0){
        config->num_channels = pac193x_get_number_of_channels(config);
    }
    config->init_done = pac193x_check_product_id(config) && pac193x_check_manufacturer_id(config);
    if(!config->init_done) {
        return false;
    }

    // Send to control register (all params will be set)
    pac193x_set_polarity(config, config->enable_bipolar_current, config->enable_bipolar_voltage);
    pac193x_set_sampling_rate(config, config->sample_rate);
    pac193x_enable_all_channels(config, config->enable_channels);
    return config->init_done;
}


bool pac193x_do_conversion(pac193x_t *config){
    return pac193x_send_refresh(config);
}


uint16_t pac193x_read_data_single(pac193x_t *config, uint8_t reg){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {reg};
    uint8_t data_rx[2] = {0x00};
    if(pac193x_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        return (data_rx[0] << 8) | (data_rx[1] << 0);
    }
    return 0;
}


bool pac193x_read_data_all_channels(pac193x_t *config, uint8_t reg_start, uint16_t *data){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {reg_start};
    uint8_t data_rx[8];
    bool state = pac193x_i2c_read(config, data_tx, sizeof(data_tx), data_rx, 2*config->num_channels);

    for(size_t idx = 0; idx < config->num_channels; idx ++){
        if(state) {
            data[idx] = (data_rx[2*idx] << 8) | (data_rx[2*idx+1] << 0);
        } else {
            data[idx] = 0;
        }
    }
    return state;
}


uint16_t pac193x_read_voltage_single(pac193x_t *config, bool take_rolling, uint8_t channel){
    if(take_rolling)
        return pac193x_read_data_single(config, PAC193X_REG_VBUS_AVG + channel);
    else
        return pac193x_read_data_single(config, PAC193X_REG_VBUS + channel);
}


bool pac193x_read_voltage_all(pac193x_t *config, bool take_rolling, uint16_t *data){
    if(take_rolling)
        return pac193x_read_data_all_channels(config, PAC193X_REG_VBUS_AVG, data);
    else
        return pac193x_read_data_all_channels(config, PAC193X_REG_VBUS, data);
}


uint16_t pac193x_read_current_single(pac193x_t *config, bool take_rolling, uint8_t channel){
    if(take_rolling)
        return pac193x_read_data_single(config, PAC193X_REG_VSENSE_AVG + channel);
    else
        return pac193x_read_data_single(config, PAC193X_REG_VSENSE + channel);
}


bool pac193x_read_current_all(pac193x_t *config, bool take_rolling, uint16_t *data){
    if(take_rolling)
        return pac193x_read_data_all_channels(config, PAC193X_REG_VSENSE_AVG, data);
    else
        return pac193x_read_data_all_channels(config, PAC193X_REG_VSENSE, data);
}


uint32_t pac193x_read_power_single(pac193x_t *config, uint8_t channel){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC193X_REG_VPOWER + channel;

    uint8_t data_rx[4] = {0x00};
    if(pac193x_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        return (data_rx[0] << 20) | (data_rx[1] << 12) | (data_rx[2] << 4) | (data_rx[3] >> 4);
    }
    return 0;
}


bool pac193x_read_power_all(pac193x_t *config, uint32_t *data){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {PAC193X_REG_VPOWER_ACC};
    uint8_t data_rx[16];
    bool state = pac193x_i2c_read(config, data_tx, sizeof(data_tx), data_rx, 4 * config->num_channels);

    for(size_t idx = 0; idx < config->num_channels; idx ++){
        if(state) {
            data[idx] = (data_rx[4*idx+0] << 20) | (data_rx[4*idx+1] << 12) | (data_rx[4*idx+2] << 4) | (data_rx[4*idx+3] >> 4);
        } else {
            data[idx] = 0;
        }
    }
    return state;
}


uint64_t pac193x_read_power_accumulated(pac193x_t *config, uint8_t channel){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC193X_REG_VPOWER_ACC + channel;

    uint8_t data_rx[6] = {0x00};
    if(pac193x_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        return ((uint64_t)data_rx[0] << 40) | ((uint64_t)data_rx[1] << 32) | ((uint64_t)data_rx[2] << 24) | ((uint64_t)data_rx[3] << 16) | ((uint64_t)data_rx[4] << 8) | (data_rx[5] << 0);
    }
    return 0;
}


uint32_t pac193x_read_accumulation_number(pac193x_t *config){
    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC193X_REG_ACC_CNT;

    uint8_t data_rx[3] = {0x00};
    if(pac193x_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        return (data_rx[0] << 16) | (data_rx[1] << 8) | (data_rx[2] << 0);
    }
    return 0;
}
