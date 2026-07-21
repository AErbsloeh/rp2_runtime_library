#include "adc/ad4858/ad4858.h"
#include "adc/ad4858/ad4858_spi.h"
#include "hal/spi/spi.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

// ================= REGISTER DEFINITIONS =================
#define REG_SPI_CONFIG_A            0x00
#define REG_SPI_CONFIG_B            0x01
#define REG_SPI_CONFIG_D            0x14

#define SPICFGA_CSDO_EN_MSK         (1u << 4)
#define SPICFGB_SINGLE_INST_MSK     (1u << 7)
#define SPICFGD_CSDO_ON_SDO0_MSK    (1u << 0)

#define MAX_FRAME_BYTES             32
#define AD4858_SPI_NUM_CHANNELS     8

//  ================= FUNCS FROM BASIC MODULE =================
extern void ad4858_set_4_wire(ad4858_t *config, bool enable);
extern void ad4858_enable_streaming_mode(ad4858_t *config, bool enable);
extern void ad4858_set_spi_output_on_sdo0(ad4858_t* config, bool enable);

// ================= INTERNAL FUNCS =================

static uint16_t frame_size_bytes(ad4858_spi_t* device){
    switch (device->package_size) {
        case AD4858_PACKETSIZE_20BIT:
            return (20u * AD4858_SPI_NUM_CHANNELS) / 8u;
        case AD4858_PACKETSIZE_24BIT:
            return (24u * AD4858_SPI_NUM_CHANNELS) / 8u;
        case AD4858_PACKETSIZE_32BIT:
            return (32u * AD4858_SPI_NUM_CHANNELS) / 8u;
        default:
            return 0;
    }
}

static bool trigger_conversion_and_wait(ad4858_spi_t* device){
    gpio_put(device->gpio_convert, true);
    sleep_us(1);
    gpio_put(device->gpio_convert, false);

    uint32_t timeout = 100000;
    while (gpio_get(device->gpio_busy)) {
        if (--timeout == 0) {
            return false;
        }
    }
    return true;
}


// ================= CALLABLE FUNCS =================
bool ad4858_spi_read_data(ad4858_t* config, ad4858_spi_t* device, ad4858_spi_conv_data_t* data){
    uint16_t nb_bytes = frame_size_bytes(device);
    if (nb_bytes == 0 || nb_bytes > MAX_FRAME_BYTES) {
        return false;
    }

    uint8_t tx[MAX_FRAME_BYTES] = {0};
    uint8_t rx[MAX_FRAME_BYTES] = {0};

    int8_t ret = receive_data_spi_module(config->spi_mod, config->gpio_csn, tx, rx, nb_bytes);
    if (ret < (int8_t)nb_bytes) {
        return false;
    }

    switch (device->package_size) {
        case AD4858_PACKETSIZE_20BIT: {
            static const uint8_t offs[AD4858_SPI_NUM_CHANNELS] = {0, 2, 5, 7, 10, 12, 15, 17};
            for (uint8_t ch = 0; ch < AD4858_SPI_NUM_CHANNELS; ch++) {
                uint32_t v = ((uint32_t)rx[offs[ch]] << 16) |
                             ((uint32_t)rx[offs[ch] + 1] << 8) |
                             (uint32_t)rx[offs[ch] + 2];
                data->raw[ch] = (ch % 2 == 0) ? ((v >> 4) & 0xFFFFFu) : (v & 0xFFFFFu);
                data->or_ur_status[ch] = 0;
                data->chn_id[ch] = 0;
            }
            break;
        }
        case AD4858_PACKETSIZE_24BIT: {
            for (uint8_t ch = 0; ch < AD4858_SPI_NUM_CHANNELS; ch++) {
                uint8_t idx = (uint8_t)(ch * 3u);
                uint32_t v = ((uint32_t)rx[idx] << 16) |
                             ((uint32_t)rx[idx + 1] << 8) |
                             (uint32_t)rx[idx + 2];
                data->raw[ch] = (v >> 4) & 0xFFFFFu;
                data->or_ur_status[ch] = (uint8_t)((rx[idx + 2] >> 3) & 0x01u);
                data->chn_id[ch] = (uint8_t)(rx[idx + 2] & 0x07u);
            }
            break;
        }
        case AD4858_PACKETSIZE_32BIT: {
            for (uint8_t ch = 0; ch < AD4858_SPI_NUM_CHANNELS; ch++) {
                uint8_t idx = (uint8_t)(ch * 4u);
                uint32_t v = ((uint32_t)rx[idx] << 16) |
                             ((uint32_t)rx[idx + 1] << 8) |
                             (uint32_t)rx[idx + 2];
                data->raw[ch] = (v >> 4) & 0xFFFFFu;
                data->or_ur_status[ch] = (uint8_t)((rx[idx + 2] >> 3) & 0x01u);
                data->chn_id[ch] = (uint8_t)(rx[idx + 2] & 0x07u);
            }
            break;
        }
        default:
            return false;
    }

    return true;
}


bool ad4858_spi_single_conversion(ad4858_t* config, ad4858_spi_t* device, ad4858_spi_conv_data_t* data){
    if (!trigger_conversion_and_wait(device)) {
        return false;
    }
    return ad4858_spi_read_data(config, device, data);
}


int32_t ad4858_spi_extract_result20(uint32_t raw20){
    int32_t v = (int32_t)(raw20 & 0xFFFFFu);
    if (v & 0x80000) {
        v -= (1 << 20);
    }
    return v;
}


bool ad4858_spi_init(ad4858_t* config, ad4858_spi_t* device){
    if(!config->init_done){
        if(!ad4858_init(config))
            return false;
    } else {
        ad4858_set_4_wire(config, config->use_4wire_spi);
        ad4858_set_spi_output_on_sdo0(config, false);
    }

    gpio_init(device->gpio_convert);
    gpio_set_dir(device->gpio_convert, GPIO_OUT);
    gpio_put(device->gpio_convert, false);

    gpio_init(device->gpio_busy);
    gpio_set_dir(device->gpio_busy, GPIO_IN);
    
    device->package_size = ad4858_get_packet_size(config);
    ad4858_enable_streaming_mode(config, true);
    return true;
}


bool ad4858_spi_deinit(ad4858_t* config, ad4858_spi_t* device){
    gpio_put(device->gpio_convert, false);

    ad4858_enable_streaming_mode(config, false);
    return true;
}
