#include "sens/pac1954/pac1954.h"
#include <stdio.h>

// More details in datasheet (PAC1951/2/3/4 Family Data Sheet).
#define PAC1954_I2C_ADDR_START 0x10
#define PAC1954_I2C_ADDR_END   0x1F

#define PAC1954_REG_REFRESH             0x00   // 0 byte
#define PAC1954_REG_CONTROL             0x01   // 2 byte  (PAC193x: 1 byte!)
#define PAC1954_REG_ACC_CNT             0x02   // 4 byte  (PAC193x: 3 byte!)
#define PAC1954_REG_VACC1               0x03   // 7 byte per channel (56-bit, PAC193x: 6 byte/48-bit)
#define PAC1954_REG_VBUS                0x07   // 2 byte per channel
#define PAC1954_REG_VSENSE              0x0B   // 2 byte per channel
#define PAC1954_REG_VBUS_AVG            0x0F   // 2 byte per channel
#define PAC1954_REG_VSENSE_AVG          0x13   // 2 byte per channel
#define PAC1954_REG_VPOWER              0x17   // 4 byte per channel
#define PAC1954_REG_SMBUS_SETTINGS      0x1C   // 1 byte  (replaces PAC193x ENABLE register!)
#define PAC1954_REG_NEG_PWR_FSR         0x1D   // 2 byte  (replaces PAC193x POLARITY register, was 1 byte!)
#define PAC1954_REG_REFRESH_G           0x1E   // 0 byte  (refresh without resetting the accumulator)
#define PAC1954_REG_REFRESH_V           0x1F   // 0 byte  (refresh voltage/current registers only)
#define PAC1954_REG_SLOW                0x20   // 1 byte  (SLOW pin control/status)
#define PAC1954_REG_CONTROL_ACT         0x21   // 2 byte  (active CTRL shadow value)
#define PAC1954_REG_NEG_PWR_FSR_ACT     0x22   // 2 byte
#define PAC1954_REG_CONTROL_LAT         0x23   // 2 byte  (CTRL value latched since last refresh)
#define PAC1954_REG_NEG_PWR_FSR_LAT     0x24   // 2 byte
#define PAC1954_REG_ACCUM_CONFIG        0x25   // 1 byte  (new, does not exist on PAC193x)
#define PAC1954_REG_ALERT_STATUS        0x26   // 3 byte
#define PAC1954_REG_SLOW_ALERT1         0x27   // 3 byte
#define PAC1954_REG_GPIO_ALERT2         0x28   // 3 byte
#define PAC1954_REG_ACC_FULLNESS_LIMITS 0x29   // 2 byte
#define PAC1954_REG_OC_LIMIT1           0x30   // 2 byte per channel (0x30-0x33)
#define PAC1954_REG_UC_LIMIT1           0x34   // 2 byte per channel (0x34-0x37)
#define PAC1954_REG_OP_LIMIT1           0x38   // 3 byte per channel (0x38-0x3B)
#define PAC1954_REG_OV_LIMIT1           0x3C   // 2 byte per channel (0x3C-0x3F)
#define PAC1954_REG_UV_LIMIT1           0x40   // 2 byte per channel (0x40-0x43)
#define PAC1954_REG_OC_LIMIT_NSAMPLES   0x44   // 1 byte
#define PAC1954_REG_UC_LIMIT_NSAMPLES   0x45   // 1 byte
#define PAC1954_REG_OP_LIMIT_NSAMPLES   0x46   // 1 byte
#define PAC1954_REG_OV_LIMIT_NSAMPLES   0x47   // 1 byte
#define PAC1954_REG_UV_LIMIT_NSAMPLES   0x48   // 1 byte
#define PAC1954_REG_ALERT_ENABLE        0x49   // 3 byte
#define PAC1954_REG_ACCUM_CONFIG_ACT    0x4A   // 1 byte
#define PAC1954_REG_ACCUM_CONFIG_LAT    0x4B   // 1 byte
#define PAC1954_REG_PID                 0xFD   // 1 byte
#define PAC1954_REG_MID                 0xFE   // 1 byte
#define PAC1954_REG_RID                 0xFF   // 1 byte

// Product ID values (PAC195x family)
#define PAC1954_PID_PAC1951    0x78   // 1 channel
#define PAC1954_PID_PAC1952_1  0x79   // 2 channels (high-side)
#define PAC1954_PID_PAC1953    0x7A   // 3 channels
#define PAC1954_PID_PAC1954    0x7B   // 4 channels
#define PAC1954_PID_PAC1951_2  0x7C   // 1 channel (high-/low-side)
#define PAC1954_PID_PAC1952_2  0x7D   // 2 channels (high-/low-side)

#define PAC1954_MID_MICROCHIP  0x5D


// ======================= INTERNAL FUNCS =======================
bool pac1954_i2c_write(pac1954_t *config, uint8_t *data, size_t len){
    return construct_i2c_write_data(config->i2c, config->adr, data, len);
}


bool pac1954_i2c_read(pac1954_t *config, uint8_t *data_tx, size_t len_tx, uint8_t *data_rx, size_t len_rx){
    return construct_i2c_read_data(config->i2c, config->adr, data_tx, len_tx, data_rx, len_rx);
}


bool pac1954_send_refresh(pac1954_t *config){
    uint8_t cmd[1] = {0x00};
    cmd[0] = PAC1954_REG_REFRESH;

    return pac1954_i2c_write(config, cmd, sizeof(cmd));
}


bool pac1954_send_refresh_v(pac1954_t *config){
    uint8_t cmd[1] = {0x00};
    cmd[0] = PAC1954_REG_REFRESH_V;

    return pac1954_i2c_write(config, cmd, sizeof(cmd));
}


bool pac1954_send_refresh_g(pac1954_t *config){
    uint8_t cmd[1] = {0x00};
    cmd[0] = PAC1954_REG_REFRESH_G;

    return pac1954_i2c_write(config, cmd, sizeof(cmd));
}


// ======================= CALLABLE FUNCS =======================
void pac1954_wait(void){
    sleep_us(600);
}


uint8_t pac1954_get_i2c_address(uint32_t resistor_value){
    if (resistor_value <= 250)
        return PAC1954_I2C_ADDR_START + 0x00;    // GND (0 Ohm) -> Default
    else if (resistor_value <= 652)
        return PAC1954_I2C_ADDR_START + 0x01;    // 499
    else if (resistor_value <= 1038)
        return PAC1954_I2C_ADDR_START + 0x02;    // 806
    else if (resistor_value <= 1660)
        return PAC1954_I2C_ADDR_START + 0x03;    // 1270
    else if (resistor_value <= 2645)
        return PAC1954_I2C_ADDR_START + 0x04;    // 2050
    else if (resistor_value <= 4235)
        return PAC1954_I2C_ADDR_START + 0x05;    // 3240
    else if (resistor_value <= 6840)
        return PAC1954_I2C_ADDR_START + 0x06;    // 5230
    else if (resistor_value <= 10875)
        return PAC1954_I2C_ADDR_START + 0x07;    // 8450
    else if (resistor_value <= 17400)
        return PAC1954_I2C_ADDR_START + 0x08;    // 13300
    else if (resistor_value <= 27750)
        return PAC1954_I2C_ADDR_START + 0x09;    // 21500
    else if (resistor_value <= 44450)
        return PAC1954_I2C_ADDR_START + 0x0A;    // 34000
    else if (resistor_value <= 71800)
        return PAC1954_I2C_ADDR_START + 0x0B;    // 54900
    else if (resistor_value <= 114350)
        return PAC1954_I2C_ADDR_START + 0x0C;    // 88700
    else if (resistor_value <= 183000)
        return PAC1954_I2C_ADDR_START + 0x0D;    // 140000
    else
        return PAC1954_I2C_ADDR_END;    // VDD (high resistance)
}


bool pac1954_check_product_id(pac1954_t *config){
    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC1954_REG_PID;

    uint8_t data_rx[1] = {0x00};
    if(pac1954_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        if(config->num_channels == 1)       return (data_rx[0] == PAC1954_PID_PAC1951) || (data_rx[0] == PAC1954_PID_PAC1951_2);
        else if(config->num_channels == 2)  return (data_rx[0] == PAC1954_PID_PAC1952_1) || (data_rx[0] == PAC1954_PID_PAC1952_2);
        else if(config->num_channels == 3)  return data_rx[0] == PAC1954_PID_PAC1953;
        else if(config->num_channels == 4)  return data_rx[0] == PAC1954_PID_PAC1954;
        else return false;
    }
    return false;
}


bool pac1954_check_manufacturer_id(pac1954_t *config){
    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC1954_REG_MID;

    uint8_t data_rx[1] = {0x00};
    if(pac1954_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        return data_rx[0] == PAC1954_MID_MICROCHIP;
    }
    return false;
}


bool pac1954_read_revision_id(pac1954_t *config, uint8_t *revision_id){
    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC1954_REG_RID;

    uint8_t data_rx[1] = {0x00};
    if(pac1954_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        *revision_id = data_rx[0];
        return true;
    }
    return false;
}


uint8_t pac1954_get_number_of_channels(pac1954_t *config){
    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC1954_REG_PID;

    uint8_t data_rx[1] = {0x00};
    if(pac1954_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        if((data_rx[0] == PAC1954_PID_PAC1951) || (data_rx[0] == PAC1954_PID_PAC1951_2))        return 1;
        else if((data_rx[0] == PAC1954_PID_PAC1952_1) || (data_rx[0] == PAC1954_PID_PAC1952_2)) return 2;
        else if(data_rx[0] == PAC1954_PID_PAC1953)                                               return 3;
        else if(data_rx[0] == PAC1954_PID_PAC1954)                                               return 4;
        else return 0;
    }
    return 0;
}


bool pac1954_set_sample_mode(pac1954_t *config, uint8_t sample_mode){
    if(sample_mode > 0x0F) {
        return false;
    }

    config->sample_mode = sample_mode;

    // NOTE - ASSUMPTION: CTRL is a 16-bit register. It is assumed the 4-bit
    // sample mode occupies bits [7:4] of the MSB (mirroring the position of
    // the sample-rate field in the PAC193x's 1-byte CTRL register), with all
    // remaining bits reserved/0. Please verify against the datasheet,
    // especially if alert-pin or other configuration bits are needed.
    uint8_t data[3] = {0x00};
    data[0] = PAC1954_REG_CONTROL;
    data[1] = (config->sample_mode & 0x0F) << 4;   // MSB
    data[2] = 0x00;                                 // LSB (reserved)

    pac1954_i2c_write(config, data, sizeof(data));
    return pac1954_send_refresh(config);
}


bool pac1954_set_accum_config(pac1954_t *config, uint8_t accum_config){
    if(accum_config > 0x03) {
        return false;
    }

    config->accum_config = accum_config;

    // NOTE - ASSUMPTION: ACCUM_CONFIG is a 1-byte register with 2 bits per
    // channel (channel 1 in bits [1:0], channel 4 in bits [7:6]). The same
    // value is applied to all channels here. Please verify against the
    // datasheet.
    uint8_t data[2] = {0x00};
    data[0] = PAC1954_REG_ACCUM_CONFIG;
    data[1] = (config->accum_config << 0) | (config->accum_config << 2) |
              (config->accum_config << 4) | (config->accum_config << 6);

    pac1954_i2c_write(config, data, sizeof(data));
    return pac1954_send_refresh(config);
}


bool pac1954_set_polarity(pac1954_t *config, bool use_bipolar_current, bool use_bipolar_voltage){
    config->enable_bipolar_voltage = use_bipolar_voltage;
    config->enable_bipolar_current = use_bipolar_current;

    // NOTE - ASSUMPTION: NEG_PWR_FSR is a 16-bit register. It is assumed the
    // low byte mirrors the PAC193x's 1-byte POLARITY register layout
    // (bit[3:0] = bipolar voltage per channel, bit[7:4] = bipolar current per
    // channel), with the high byte written as 0 (no extended negative
    // full-scale-range used). Please verify against the datasheet.
    uint8_t data[3] = {0x00};
    data[0] = PAC1954_REG_NEG_PWR_FSR;
    data[1] = 0x00;    // MSB (reserved / extended FSR range, unused)
    data[2] = ((config->enable_bipolar_voltage) ? 0x0F : 0x00) |
              ((config->enable_bipolar_current) ? 0xF0 : 0x00);    // LSB

    pac1954_i2c_write(config, data, sizeof(data));
    return pac1954_send_refresh(config);
}


bool pac1954_init(pac1954_t *config){
    if(config->gpio_pwrdwn != 255){
        gpio_init(config->gpio_pwrdwn);
        gpio_set_dir(config->gpio_pwrdwn, GPIO_OUT);
        gpio_pull_up(config->gpio_pwrdwn);
        gpio_put(config->gpio_pwrdwn, false);
        sleep_us(1000);
        gpio_put(config->gpio_pwrdwn, true);
        sleep_us(1000);
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
    if((config->adr == 0) || (config->adr < PAC1954_I2C_ADDR_START) || (config->adr > PAC1954_I2C_ADDR_END)){
        return false;
    }
    if(!check_i2c_bus_for_device_specific(config->i2c, config->adr)){
        return false;
    }
    if(!pac1954_check_product_id(config)) {
        return false;
    } else {
        bool state = true;
        state &= pac1954_set_polarity(config, config->enable_bipolar_current, config->enable_bipolar_voltage);
        pac1954_wait();
        state &= pac1954_set_accum_config(config, config->accum_config);
        pac1954_wait();
        state &= pac1954_set_sample_mode(config, config->sample_mode);
        pac1954_wait();
        config->init_done = state;
        return config->init_done;
    }
    return true;
}


bool pac1954_do_conversion(pac1954_t *config){
    return pac1954_send_refresh(config);
}


bool pac1954_do_conversion_voltage_only(pac1954_t *config){
    return pac1954_send_refresh_v(config);
}


bool pac1954_do_conversion_no_reset(pac1954_t *config){
    return pac1954_send_refresh_g(config);
}


uint16_t pac1954_read_data_single(pac1954_t *config, uint8_t reg){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {reg};
    uint8_t data_rx[2] = {0x00};
    if(pac1954_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        return ((uint16_t)data_rx[0] << 8) | ((uint16_t)data_rx[1] << 0);
    } else {
        return 0;
    }
}


bool pac1954_read_data_all_channels(pac1954_t *config, uint8_t reg_start, uint16_t *data){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {reg_start};
    uint8_t data_rx[8];
    bool state = pac1954_i2c_read(config, data_tx, sizeof(data_tx), data_rx, 2*config->num_channels);

    for(size_t idx = 0; idx < config->num_channels; idx ++){
        if(state) {
            data[idx] = ((uint16_t)data_rx[2*idx] << 8) | ((uint16_t)data_rx[2*idx+1] << 0);
        } else {
            data[idx] = 0;
        }
    }
    return state;
}


uint16_t pac1954_read_voltage_single(pac1954_t *config, bool take_rolling, uint8_t channel){
    if(take_rolling)
        return pac1954_read_data_single(config, PAC1954_REG_VBUS_AVG + channel);
    else
        return pac1954_read_data_single(config, PAC1954_REG_VBUS + channel);
}


bool pac1954_read_voltage_all(pac1954_t *config, bool take_rolling, uint16_t *data){
    if(take_rolling)
        return pac1954_read_data_all_channels(config, PAC1954_REG_VBUS_AVG, data);
    else
        return pac1954_read_data_all_channels(config, PAC1954_REG_VBUS, data);
}


uint16_t pac1954_read_current_single(pac1954_t *config, bool take_rolling, uint8_t channel){
    if(take_rolling)
        return pac1954_read_data_single(config, PAC1954_REG_VSENSE_AVG + channel);
    else
        return pac1954_read_data_single(config, PAC1954_REG_VSENSE + channel);
}


bool pac1954_read_current_all(pac1954_t *config, bool take_rolling, uint16_t *data){
    if(take_rolling)
        return pac1954_read_data_all_channels(config, PAC1954_REG_VSENSE_AVG, data);
    else
        return pac1954_read_data_all_channels(config, PAC1954_REG_VSENSE, data);
}


uint32_t pac1954_read_power_single(pac1954_t *config, uint8_t channel){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC1954_REG_VPOWER + channel;

    uint8_t data_rx[4] = {0x00};
    if(pac1954_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        return ((uint32_t)data_rx[0] << 20) | ((uint32_t)data_rx[1] << 12) | ((uint32_t)data_rx[2] << 4) | ((uint32_t)data_rx[3] >> 4);
    } else {
        return 0;
    }
}


bool pac1954_read_power_all(pac1954_t *config, uint32_t *data){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {PAC1954_REG_VPOWER};
    uint8_t data_rx[16];
    bool state = pac1954_i2c_read(config, data_tx, sizeof(data_tx), data_rx, 4 * config->num_channels);

    for(size_t idx = 0; idx < config->num_channels; idx ++){
        if(state) {
            data[idx] = ((uint32_t)data_rx[4*idx+0] << 20) | ((uint32_t)data_rx[4*idx+1] << 12) | ((uint32_t)data_rx[4*idx+2] << 4) | ((uint32_t)data_rx[4*idx+3] >> 4);
        } else {
            data[idx] = 0;
        }
    }
    return state;
}


uint64_t pac1954_read_power_accumulated_single(pac1954_t *config, uint8_t channel){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC1954_REG_VACC1 + channel;

    uint8_t data_rx[7] = {0x00};
    if(pac1954_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        return ((uint64_t)data_rx[0] << 48) | ((uint64_t)data_rx[1] << 40) | ((uint64_t)data_rx[2] << 32) |
               ((uint64_t)data_rx[3] << 24) | ((uint64_t)data_rx[4] << 16) | ((uint64_t)data_rx[5] << 8)  |
               ((uint64_t)data_rx[6] << 0);
    } else {
        return 0;
    }
}


bool pac1954_read_power_accumulated_all(pac1954_t *config, uint64_t *data){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC1954_REG_VACC1;
    uint8_t data_rx[28] = {0x00};
    bool state = pac1954_i2c_read(config, data_tx, sizeof(data_tx), data_rx, 7 * config->num_channels);

    for(size_t idx = 0; idx < config->num_channels; idx ++){
        if(state) {
            data[idx] = ((uint64_t)data_rx[7*idx+0] << 48) | ((uint64_t)data_rx[7*idx+1] << 40) | ((uint64_t)data_rx[7*idx+2] << 32) |
                        ((uint64_t)data_rx[7*idx+3] << 24) | ((uint64_t)data_rx[7*idx+4] << 16) | ((uint64_t)data_rx[7*idx+5] << 8)  |
                        ((uint64_t)data_rx[7*idx+6] << 0);
        } else {
            data[idx] = 0;
        }
    }
    return state;
}


uint32_t pac1954_read_accumulation_number(pac1954_t *config){
    if(!config->init_done)
        return 0;

    uint8_t data_tx[1] = {0x00};
    data_tx[0] = PAC1954_REG_ACC_CNT;

    uint8_t data_rx[4] = {0x00};
    if(pac1954_i2c_read(config, data_tx, sizeof(data_tx), data_rx, sizeof(data_rx))){
        return ((uint32_t)data_rx[0] << 24) | ((uint32_t)data_rx[1] << 16) | ((uint32_t)data_rx[2] << 8) | ((uint32_t)data_rx[3] << 0);
    } else {
        return 0;
    }
}
