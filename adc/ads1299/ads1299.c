#include "adc/ads1299/ads1299.h"

#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// More details in datasheet SBAS499 (ADS1299 8-Channel, 24-Bit ADC for EEG/ECG).
// See the note at the top of ads1299.h regarding sources and assumptions.

// ---- SPI command bytes (datasheet, "Command Definitions") ----
#define ADS1299_CMD_WAKEUP   0x02
#define ADS1299_CMD_STANDBY  0x04
#define ADS1299_CMD_RESET    0x06
#define ADS1299_CMD_START    0x08
#define ADS1299_CMD_STOP     0x0A
#define ADS1299_CMD_RDATAC   0x10
#define ADS1299_CMD_SDATAC   0x11
#define ADS1299_CMD_RDATA    0x12
#define ADS1299_CMD_RREG     0x20   // OR with register address (001rrrrr)
#define ADS1299_CMD_WREG     0x40   // OR with register address (010rrrrr)

// ---- Register addresses (0x00 - 0x17) ----
#define ADS1299_REG_ID           0x00
#define ADS1299_REG_CONFIG1      0x01
#define ADS1299_REG_CONFIG2      0x02
#define ADS1299_REG_CONFIG3      0x03
#define ADS1299_REG_LOFF         0x04
#define ADS1299_REG_CH1SET       0x05
#define ADS1299_REG_CH2SET       0x06
#define ADS1299_REG_CH3SET       0x07
#define ADS1299_REG_CH4SET       0x08
#define ADS1299_REG_CH5SET       0x09
#define ADS1299_REG_CH6SET       0x0A
#define ADS1299_REG_CH7SET       0x0B
#define ADS1299_REG_CH8SET       0x0C
#define ADS1299_REG_BIAS_SENSP   0x0D
#define ADS1299_REG_BIAS_SENSN   0x0E
#define ADS1299_REG_LOFF_SENSP   0x0F
#define ADS1299_REG_LOFF_SENSN   0x10
#define ADS1299_REG_LOFF_FLIP    0x11
#define ADS1299_REG_LOFF_STATP   0x12
#define ADS1299_REG_LOFF_STATN   0x13
#define ADS1299_REG_GPIO         0x14
#define ADS1299_REG_MISC1        0x15
#define ADS1299_REG_MISC2        0x16
#define ADS1299_REG_CONFIG4      0x17

#define ADS1299_NUM_REGISTERS    0x18

// NOTE - ASSUMPTION: reserved/fixed bit patterns below are taken from the
// commonly documented power-up default values of the respective registers.
// CONFIG1: bit7=1 (fixed), bit4=1 (fixed), bits[2:0]=DR[2:0] (data rate).
#define ADS1299_CONFIG1_BASE     0x90
// CONFIG2: written as a fixed default (bit7=1, bit6=1 fixed), no test-signal
// configuration exposed by this driver.
#define ADS1299_CONFIG2_DEFAULT  0xC0
// CONFIG3: bit6=1 (fixed/reserved, must be 1 per datasheet). bit7=PD_REFBUF
// (1=internal reference buffer enabled), bit4=BIASREF_INT (1=internal bias
// reference), bit3=PD_BIAS (1=BIAS buffer enabled).
#define ADS1299_CONFIG3_BASE       0x40
#define ADS1299_CONFIG3_PD_REFBUF   0x80
#define ADS1299_CONFIG3_BIASREF_INT 0x10
#define ADS1299_CONFIG3_PD_BIAS     0x08

// CHnSET: bit7=PD (0=on), bits[6:4]=GAIN[2:0], bit3=SRB2 (1=closed),
// bits[2:0]=MUX[2:0] (000=normal electrode input).
#define ADS1299_CHSET_SRB2_CLOSED   0x08

// NOTE - ASSUMPTION: commonly documented device ID for the 8-channel
// ADS1299 (bits[4:2]=DEV_ID, bits[1:0]=channel-count code). Not enforced as
// a hard check in ads1299_read_device_id(), see header docstring.
#define ADS1299_ID_8CH  0x3E

// Approximate conversion period per data rate setting, used only as a
// worst-case busy-wait fallback if gpio_drdy is unused (255). Values are
// derived from fMOD = fCLK / 4 with fCLK = 2.048 MHz (internal oscillator).
// NOTE - ASSUMPTION: if an external clock is used, these periods are wrong;
// wire DRDY for reliable timing.
static const uint32_t ADS1299_PERIOD_US[7] = {
    63,     // ADS1299_RATE_16KSPS
    125,    // ADS1299_RATE_8KSPS
    250,    // ADS1299_RATE_4KSPS
    500,    // ADS1299_RATE_2KSPS
    1000,   // ADS1299_RATE_1KSPS
    2000,   // ADS1299_RATE_500SPS
    4000,   // ADS1299_RATE_250SPS
};


// ======================= INTERNAL FUNCS =======================
bool ads1299_send_command(ads1299_t *config, uint8_t command){
    uint8_t data[1] = {command};
    return send_data_spi_module(config->spi, config->gpio_cs, data, sizeof(data)) >= 0;
}


// ======================= CALLABLE FUNCS =======================
void ads1299_wait(void){
    sleep_us(10);
}


bool ads1299_wakeup(ads1299_t *config){
    return ads1299_send_command(config, ADS1299_CMD_WAKEUP);
}


bool ads1299_standby(ads1299_t *config){
    return ads1299_send_command(config, ADS1299_CMD_STANDBY);
}


bool ads1299_reset(ads1299_t *config){
    if(config->gpio_reset != 255){
        gpio_put(config->gpio_reset, false);
        sleep_us(10);
        gpio_put(config->gpio_reset, true);
        return true;
    }
    return ads1299_send_command(config, ADS1299_CMD_RESET);
}


bool ads1299_start(ads1299_t *config){
    if(config->gpio_start != 255){
        gpio_put(config->gpio_start, true);
        return true;
    }
    return ads1299_send_command(config, ADS1299_CMD_START);
}


bool ads1299_stop(ads1299_t *config){
    if(config->gpio_start != 255){
        gpio_put(config->gpio_start, false);
        return true;
    }
    return ads1299_send_command(config, ADS1299_CMD_STOP);
}


bool ads1299_rdatac_enable(ads1299_t *config){
    bool state = ads1299_send_command(config, ADS1299_CMD_RDATAC);
    config->rdatac_mode = true;
    return state;
}


bool ads1299_rdatac_disable(ads1299_t *config){
    bool state = ads1299_send_command(config, ADS1299_CMD_SDATAC);
    config->rdatac_mode = false;
    ads1299_wait();   // datasheet: wait >= 4 tCLK after SDATAC before the next command
    return state;
}


bool ads1299_read_registers(ads1299_t *config, uint8_t reg_addr_start, uint8_t *data, uint8_t num_registers){
    if((num_registers == 0) || (num_registers > ADS1299_NUM_REGISTERS)){
        return false;
    }

    uint8_t tx[2 + ADS1299_NUM_REGISTERS] = {0x00};
    uint8_t rx[2 + ADS1299_NUM_REGISTERS] = {0x00};
    size_t len = 2 + num_registers;

    tx[0] = ADS1299_CMD_RREG | (reg_addr_start & 0x1F);
    tx[1] = num_registers - 1;

    bool state = receive_data_spi_module(config->spi, config->gpio_cs, tx, rx, len) >= 0;
    if(state){
        memcpy(data, &rx[2], num_registers);
    }
    return state;
}


uint8_t ads1299_read_register(ads1299_t *config, uint8_t reg_addr){
    uint8_t value = 0x00;
    ads1299_read_registers(config, reg_addr, &value, 1);
    return value;
}


bool ads1299_write_registers(ads1299_t *config, uint8_t reg_addr_start, uint8_t *data, uint8_t num_registers){
    if((num_registers == 0) || (num_registers > ADS1299_NUM_REGISTERS)){
        return false;
    }

    uint8_t tx[2 + ADS1299_NUM_REGISTERS] = {0x00};
    size_t len = 2 + num_registers;

    tx[0] = ADS1299_CMD_WREG | (reg_addr_start & 0x1F);
    tx[1] = num_registers - 1;
    memcpy(&tx[2], data, num_registers);

    return send_data_spi_module(config->spi, config->gpio_cs, tx, len) >= 0;
}


bool ads1299_write_register(ads1299_t *config, uint8_t reg_addr, uint8_t value){
    uint8_t data[1] = {value};
    return ads1299_write_registers(config, reg_addr, data, 1);
}


bool ads1299_read_device_id(ads1299_t *config, uint8_t *device_id){
    uint8_t value = 0x00;
    bool state = ads1299_read_registers(config, ADS1299_REG_ID, &value, 1);
    *device_id = value;
    return state;
}


bool ads1299_set_gain(ads1299_t *config, uint8_t gain){
    if(gain > ADS1299_GAIN_24){
        return false;
    }

    config->gain = gain;
    uint8_t value = (uint8_t)((config->gain & 0x07) << 4) | ADS1299_CHSET_SRB2_CLOSED;

    bool state = true;
    for(uint8_t ch = 0; ch < config->num_channels; ch++){
        state &= ads1299_write_register(config, ADS1299_REG_CH1SET + ch, value);
    }
    return state;
}


bool ads1299_set_data_rate(ads1299_t *config, uint8_t data_rate){
    if(data_rate > ADS1299_RATE_250SPS){
        return false;
    }

    config->data_rate = data_rate;
    uint8_t value = ADS1299_CONFIG1_BASE | (config->data_rate & 0x07);

    return ads1299_write_register(config, ADS1299_REG_CONFIG1, value);
}


bool ads1299_update_config3(ads1299_t *config){
    uint8_t value = ADS1299_CONFIG3_BASE;
    value |= (config->use_internal_reference) ? ADS1299_CONFIG3_PD_REFBUF : 0x00;
    value |= (config->enable_bias) ? (ADS1299_CONFIG3_BIASREF_INT | ADS1299_CONFIG3_PD_BIAS) : 0x00;

    return ads1299_write_register(config, ADS1299_REG_CONFIG3, value);
}


bool ads1299_set_reference(ads1299_t *config, bool use_internal_reference){
    config->use_internal_reference = use_internal_reference;
    return ads1299_update_config3(config);
}


bool ads1299_set_bias(ads1299_t *config, bool enable_bias){
    config->enable_bias = enable_bias;
    return ads1299_update_config3(config);
}


bool ads1299_init(ads1299_t *config){
    gpio_init(config->gpio_cs);
    gpio_set_dir(config->gpio_cs, GPIO_OUT);
    gpio_put(config->gpio_cs, true);

    if(config->gpio_reset != 255){
        gpio_init(config->gpio_reset);
        gpio_set_dir(config->gpio_reset, GPIO_OUT);
        gpio_put(config->gpio_reset, true);
    }
    if(config->gpio_drdy != 255){
        gpio_init(config->gpio_drdy);
        gpio_set_dir(config->gpio_drdy, GPIO_IN);
    }
    if(config->gpio_start != 255){
        gpio_init(config->gpio_start);
        gpio_set_dir(config->gpio_start, GPIO_OUT);
        gpio_put(config->gpio_start, false);
    }

    if(!config->spi->init_done){
        configure_spi_module(config->spi, false);
    }

    config->init_done = false;
    config->rdatac_mode = true;   // device is in RDATAC mode by default after power-up/reset

    ads1299_reset(config);
    sleep_us(50);   // NOTE - ASSUMPTION: conservative delay, datasheet requires >= 18 tCLK after RESET

    bool state = true;
    state &= ads1299_rdatac_disable(config);   // leave RDATAC so RREG/WREG are accepted
    ads1299_wait();
    state &= ads1299_set_data_rate(config, config->data_rate);
    ads1299_wait();
    state &= ads1299_set_reference(config, config->use_internal_reference);
    ads1299_wait();
    state &= ads1299_set_bias(config, config->enable_bias);
    ads1299_wait();
    state &= ads1299_write_register(config, ADS1299_REG_CONFIG2, ADS1299_CONFIG2_DEFAULT);
    ads1299_wait();
    state &= ads1299_set_gain(config, config->gain);
    ads1299_wait();

    uint8_t device_id = 0x00;
    state &= ads1299_read_device_id(config, &device_id);

    config->init_done = state;
    return config->init_done;
}


bool ads1299_wait_for_data_ready(ads1299_t *config, uint32_t timeout_us){
    if(config->gpio_drdy != 255){
        uint32_t start = time_us_32();
        while(gpio_get(config->gpio_drdy)){
            if((time_us_32() - start) > timeout_us){
                return false;
            }
        }
        return true;
    } else {
        // NOTE - ASSUMPTION: no DRDY pin wired, fall back to a fixed
        // worst-case delay derived from the configured data rate.
        uint32_t period = (config->data_rate <= ADS1299_RATE_250SPS) ? ADS1299_PERIOD_US[config->data_rate] : ADS1299_PERIOD_US[6];
        sleep_us(period);
        return true;
    }
}


bool ads1299_read_data_all(ads1299_t *config, uint32_t *status, int32_t *data){
    if(!config->init_done)
        return false;
    if(config->rdatac_mode)
        return false;   // RDATA is ignored while RDATAC is active, see datasheet

    size_t len_data = 3 + 3 * config->num_channels;
    uint8_t tx[1 + 3 + 3*8] = {0x00};
    uint8_t rx[1 + 3 + 3*8] = {0x00};
    size_t len = 1 + len_data;

    tx[0] = ADS1299_CMD_RDATA;

    bool state = receive_data_spi_module(config->spi, config->gpio_cs, tx, rx, len) >= 0;
    if(!state){
        return false;
    }

    *status = ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | ((uint32_t)rx[3] << 0);

    for(uint8_t ch = 0; ch < config->num_channels; ch++){
        size_t idx = 4 + 3*ch;
        uint32_t raw24 = ((uint32_t)rx[idx] << 16) | ((uint32_t)rx[idx+1] << 8) | ((uint32_t)rx[idx+2] << 0);
        if(raw24 & 0x00800000){
            raw24 |= 0xFF000000;   // sign-extend 24-bit two's complement to 32-bit
        }
        data[ch] = (int32_t)raw24;
    }
    return true;
}


int32_t ads1299_read_data_single(ads1299_t *config, uint8_t channel){
    uint32_t status = 0;
    int32_t data[8] = {0};
    if(!ads1299_read_data_all(config, &status, data)){
        return 0;
    }
    if(channel >= config->num_channels){
        return 0;
    }
    return data[channel];
}