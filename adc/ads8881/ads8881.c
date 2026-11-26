#include "adc/ads8881/ads8881.h"
#include "hardware/gpio.h"

#define ADS8881_DAISY_CHAIN_BITS_PER_ADC 18u
#define ADS8881_DAISY_CHAIN_MAX_RX_BYTES 256u
#define ADS8881_BUSY_TIMEOUT_US 1000u

static int32_t ads8881_read_signed_bits(const uint8_t *buf, size_t bit_position, uint8_t width) {
    uint32_t v = 0;

    for (uint8_t k = 0; k < width; k++) {
        size_t pos = bit_position + k;
        size_t byte_index = pos / 8;
        uint8_t bit_index = 7 - (pos % 8);

        v = (v << 1) | ((buf[byte_index] >> bit_index) & 1u);
    }

    v &= (width == 32) ? 0xFFFFFFFFu : ((1u << width) - 1u);

    return ((int32_t)(v << (32 - width))) >> (32 - width);
}

int ads8881_decode_daisy_chain(const uint8_t *buffer_rx, uint8_t num_adcs_total, int32_t *out)
{
    if (!buffer_rx || !out || num_adcs_total == 0) {
        return -1;
    }

    for (uint8_t i = 0; i < num_adcs_total; i++) {
        int32_t value = ads8881_read_signed_bits(
            buffer_rx,
            (size_t)i * ADS8881_DAISY_CHAIN_BITS_PER_ADC,
            ADS8881_DAISY_CHAIN_BITS_PER_ADC
        );

        uint8_t out_idx = num_adcs_total - 1 - i;
        out[out_idx] = value;
    }

    return num_adcs_total;
}

int ads8881_daisy_chain_wo_busy(ads8881_t *handler, uint8_t *buffer_rx, uint8_t len)
{
    gpio_put(handler->gpio_num_cnv, 0);
    sleep_us(1);

    gpio_put(handler->gpio_num_cnv, 1);
    sleep_us(1);

    int n = spi_read_blocking(handler->spi_handler->spi_mod, 0x00, buffer_rx, len);

    gpio_put(handler->gpio_num_cnv, 0);
    return n;
}

uint32_t convert_one_drqst_into_value(uint8_t *data, bool invert){
    uint32_t value = 0x00020000 ^ ((data[0] << 10) | (data[1] << 2) | (data[2] >> 6));
    return (invert) ? 0x00020000 - (value - 0x00020000) : value;
}


int ads8881_three_wire_wo_busy(ads8881_t *handler, uint8_t *buffer_rx, uint8_t len){
    gpio_put(handler->gpio_num_cnv, true);
    sleep_us(1);   
    gpio_put(handler->gpio_num_cnv, false);

    return spi_read_blocking(handler->spi_handler->spi_mod, 0x00, buffer_rx, len);
}


static bool ads8881_wait_end_of_busy(ads8881_t *handler){
    absolute_time_t t_end = make_timeout_time_us(ADS8881_BUSY_TIMEOUT_US);
    while(gpio_get(handler->spi_handler->pin_miso)){
        if(time_reached(t_end)){
            return false;
        }
    }
    return true;
}


int ads8881_three_wire_with_busy(ads8881_t *handler, uint8_t *buffer_rx, uint8_t len){
    gpio_put(handler->gpio_num_cnv, true);
    sleep_us(1);
    gpio_put(handler->gpio_num_cnv, false);

    if(!ads8881_wait_end_of_busy(handler)){
        return PICO_ERROR_TIMEOUT;
    }
    return spi_read_blocking(handler->spi_handler->spi_mod, 0x00, buffer_rx, len);
}


int ads8881_four_wire_wo_busy(ads8881_t *handler, uint8_t *buffer_rx, uint8_t len){
    gpio_put(handler->gpio_num_cnv, true);
    gpio_put(handler->gpio_num_din, true);
    sleep_us(1);   
    
    gpio_put(handler->gpio_num_din, false);
    int num_bytes = spi_read_blocking(handler->spi_handler->spi_mod, 0x00, buffer_rx, len);
    gpio_put(handler->gpio_num_din, true);
    
    gpio_put(handler->gpio_num_cnv, false);

    return num_bytes;
}


int ads8881_four_wire_with_busy(ads8881_t *handler, uint8_t *buffer_rx, uint8_t len){
    gpio_put(handler->gpio_num_cnv, true);
    sleep_us(1);
    gpio_put(handler->gpio_num_din, false);
    
    if(!ads8881_wait_end_of_busy(handler)){
        gpio_put(handler->gpio_num_din, true);
        gpio_put(handler->gpio_num_cnv, false);
        return PICO_ERROR_TIMEOUT;
    }
    int num_bytes = spi_read_blocking(handler->spi_handler->spi_mod, 0x00, buffer_rx, len);
    gpio_put(handler->gpio_num_din, true);
    gpio_put(handler->gpio_num_cnv, false);

    return num_bytes;
}


bool ads8881_init(ads8881_t *handler){
    if(!handler->spi_handler->init_done){
        configure_spi_module(handler->spi_handler, false);
    };

    if(handler->spi_handler->mode > 0){
        handler->init_done = false;
    } else {
        gpio_init(handler->gpio_num_cnv);
        gpio_set_dir(handler->gpio_num_cnv, GPIO_OUT);
        gpio_put(handler->gpio_num_cnv, false);
        handler->init_done = true;

        if(handler->adc_mode > 1){
            gpio_init(handler->gpio_num_din);
            gpio_set_dir(handler->gpio_num_din, GPIO_OUT);
            gpio_set_drive_strength(handler->gpio_num_din, GPIO_DRIVE_STRENGTH_2MA);
            gpio_put(handler->gpio_num_din, false);
        }
    }    
    return handler->init_done;
}


int ads8881_rqst_data_mode(ads8881_t *handler, uint8_t *data, uint8_t len){
    if(!handler->init_done && !ads8881_init(handler)){
        return -1;
    }

    switch(handler->adc_mode){
        case ADS8881_THREE_WIRE_WO_BUSY_IND:
            return ads8881_three_wire_wo_busy(handler, data, len);
        case ADS8881_THREE_WIRE_W_BUSY_IND:
            return ads8881_three_wire_with_busy(handler, data, len);
        case ADS8881_FOUR_WIRE_WO_BUSY_IND:
            return ads8881_four_wire_wo_busy(handler, data, len);
        case ADS8881_FOUR_WIRE_W_BUSY_IND:
            return ads8881_four_wire_with_busy(handler, data, len);
        default:
            return -1;
    }
}


uint32_t ads8881_rqst_data(ads8881_t *handler){
    uint8_t data[3];
    if(ads8881_rqst_data_mode(handler, data, 3) < 0){
        return 0;
    }
    return convert_one_drqst_into_value(data, handler->invert_out);
}


int ads8881_daisy_chain_rqst_data_wo_busy(
    ads8881_t *handler,
    uint8_t num_adcs_total,
    int32_t *out
) {
    if (!handler || !out || num_adcs_total == 0) {
        return -1;
    }

    if (!handler->init_done && !ads8881_init(handler)) {
        return -2;
    }

    size_t total_bits = (size_t)num_adcs_total * ADS8881_DAISY_CHAIN_BITS_PER_ADC;
    size_t total_bytes = (total_bits + 7u) / 8u;

    if (total_bytes > ADS8881_DAISY_CHAIN_MAX_RX_BYTES) {
        return -3;
    }

    uint8_t buffer_rx[ADS8881_DAISY_CHAIN_MAX_RX_BYTES] = {0};

    int n = ads8881_daisy_chain_wo_busy(handler, buffer_rx, (uint8_t)total_bytes);
    if (n != (int)total_bytes) {
        return -4;
    }

    return ads8881_decode_daisy_chain(buffer_rx, num_adcs_total, out);
}