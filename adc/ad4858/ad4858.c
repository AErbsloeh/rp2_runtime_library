#include "adc/ad4858/ad4858.h"


// ====================== REGISTER ADDRESSES ====================== 
#define NUM_CHANNELS        0x08

#define REG_SPI_CONFIG_A    0x00
#define REG_SPI_CONFIG_B    0x01
#define REG_SPI_CONFIG_C    0x10
#define REG_SPI_CONFIG_D    0x14

#define REG_DEVICE_TYPE     0x03
#define REG_PRODUCT_IDL     0x04
#define REG_PRODUCT_IDH     0x05
#define REG_VENDOR_IDL      0x0C
#define REG_VENDOR_IDH      0x0D
#define REG_SPI_STATUS      0x11
#define REG_DEVICE_STATUS   0x20

#define REG_DEVICE_CONFIG   0x02
#define REG_DEVICE_CTRL     0x25
#define REG_PACKET          0x26
#define REG_OVERSAMPLE      0x27
#define REG_SEAMLESS_HDR    0x28
#define REG_CHANNEL_SLEEP   0x29


static const uint8_t REG_SOFTSPAN_ADDR[NUM_CHANNELS] = {
    0x2A,  // CH0
    0x3C,  // CH1
    0x4E,  // CH2
    0x60,  // CH3
    0x72,  // CH4
    0x84,  // CH5
    0x96,  // CH6
    0xA8   // CH7
};


static uint32_t TESTPATTERN_CHANNEL[NUM_CHANNELS] = {
    0x0ACE3C2A, // CH0
    0x1ACE3C2A, // CH1
    0x2ACE3C2A, // CH2
    0x3ACE3C2A, // CH3
    0x4ACE3C2A, // CH4
    0x5ACE3C2A, // CH5
    0x6ACE3C2A, // CH6
    0x7ACE3C2A, // CH7
};


// ======================================== INTERNAL READ/WRITE COMMANDS ===============================================

uint8_t do_spi_transmission(ad4858_t *config, bool rnw, uint16_t adr, uint8_t data){
    uint8_t buffer_tx[3] = {0x00};
    uint8_t buffer_rx[3] = {0x00};
    buffer_tx[0] = ((rnw) ? 0x80 : 0x00) | ((adr & 0x7F00) >> 0x08);
    buffer_tx[1] = ((adr & 0x00FF) >> 0x00);
    buffer_tx[2] = data;

    receive_data_spi_module(config->spi_mod, config->gpio_csn, buffer_tx, buffer_rx, 3);
    return buffer_rx[2];
};


void do_spi_transmission_modified(ad4858_t *config, uint16_t adr, uint8_t data, uint8_t mask){
    uint8_t reg_data = do_spi_transmission(config, true, adr, 0x00);    //Read
    uint8_t mod = (mask & data) | (~mask & reg_data);                   //Modify
    do_spi_transmission(config, false, adr, mod);                       //Write
}


void ad4858_set_4_wire(ad4858_t *config, bool enable){
    do_spi_transmission_modified(config, REG_SPI_CONFIG_A, (enable) ? 0x30 : 0x20, 0x30);
}


void ad4858_enable_streaming_mode(ad4858_t *config, bool enable){
    do_spi_transmission(config, false, REG_SPI_CONFIG_B, (enable) ? 0x00 : 0x80);
}


void ad4858_control_crc(ad4858_t* config, bool enable){
    do_spi_transmission(config, false, REG_SPI_CONFIG_C, (enable) ? 0xC0 : 0x03);
}


void ad4858_set_spi_output_on_sdo0(ad4858_t* config, bool enable){
    do_spi_transmission(config, false, REG_SPI_CONFIG_D, (enable) ? 0x01 : 0x00);
}


void ad4858_enable_spi_sclk_echo(ad4858_t *config, bool enable){
    do_spi_transmission_modified(config, REG_DEVICE_CTRL, (enable) ? 0x01 : 0x00, 0x01);
}


void ad4858_set_reference_source(ad4858_t *config, bool use_external){
    do_spi_transmission_modified(config, REG_DEVICE_CTRL, (use_external) ? 0x02 : 0x00, 0x02);
}


void ad4858_enable_reference_buffer(ad4858_t *config, bool enable){
    do_spi_transmission_modified(config, REG_DEVICE_CTRL, (enable) ? 0x00 : 0x04, 0x04);
}


void ad4858_set_oversampling_ratio(ad4858_t *config, uint8_t osr_ratio){
    if(osr_ratio == AD4858_OSR_RATIO_1) {
        do_spi_transmission(config, false, REG_OVERSAMPLE, 0x00);
    } else {
        do_spi_transmission(config, false, REG_OVERSAMPLE, 0x80 | (osr_ratio & 0x0F));
    }
}


void ad4858_enable_seamless_hdr_mode(ad4858_t *config, bool enable){
    do_spi_transmission(config, false, REG_SEAMLESS_HDR, (enable) ? 0xFF : 0x00);
}


// ============================= CALLABLE FUNCS =============================
uint8_t ad4858_get_device_type(ad4858_t* config){
    if(config->use_4wire_spi){
        return do_spi_transmission(config, true, REG_DEVICE_TYPE, 0x00);
    } else {
        return 0;
    }
}


uint16_t ad4858_get_product_id(ad4858_t *config){
    if(config->use_4wire_spi){
        uint8_t prod_h = do_spi_transmission(config, true, REG_PRODUCT_IDH, 0x00);
        uint8_t prod_l = do_spi_transmission(config, true, REG_PRODUCT_IDL, 0x00);
        return (prod_h << 8) | prod_l;
    } else {
        return 0x0060;
    }
}


uint16_t ad4858_get_vendor_id(ad4858_t *config){
    if(config->use_4wire_spi){
        uint8_t prod_h = do_spi_transmission(config, true, REG_VENDOR_IDH, 0x00);
        uint8_t prod_l = do_spi_transmission(config, true, REG_VENDOR_IDL, 0x00);
        return (prod_h << 8) | prod_l;
    } else {
        return 0x0456;
    }
}


uint8_t ad4858_get_spi_status(ad4858_t *config){
    return do_spi_transmission(config, true, REG_SPI_STATUS, 0x00);
}


uint8_t ad4858_get_device_status(ad4858_t *config){
    return do_spi_transmission(config, true, REG_DEVICE_STATUS, 0x00);
}


void ad4858_set_packet_size(ad4858_t *config, uint8_t packet_size){
    do_spi_transmission_modified(config, REG_PACKET, packet_size, 0x03);
}


uint8_t ad4858_get_packet_size(ad4858_t* config){
    return do_spi_transmission(config, true, REG_PACKET, 0x00);
}


void ad4858_set_softspan(ad4858_t *config, uint8_t channel, uint8_t lvl){
    do_spi_transmission_modified(config, REG_SOFTSPAN_ADDR[channel], lvl, 0x0F);
}


void ad4858_set_channel_into_sleep(ad4858_t *config, uint8_t channel, bool go_sleep){
    do_spi_transmission_modified(config, REG_CHANNEL_SLEEP, (go_sleep) ? (1 << channel) : 0x00, (1 << channel));
}


void ad4858_set_power_mode(ad4858_t *config, bool enable){
    if(config->gpio_pwr_dwn != 255){
        gpio_put(config->gpio_pwr_dwn, !enable);
        sleep_us(250);
    } else {
        do_spi_transmission(config, false, REG_DEVICE_CONFIG, (enable) ? 0x00 : 0x03);
    }
}


void ad4858_enable_cmos_testpattern(ad4858_t *config, bool enable){
    do_spi_transmission_modified(config, REG_PACKET, (enable) ? 0x04 : 0x00, 0x04);
}


void ad4858_do_reset(ad4858_t *config){
    if(config->gpio_pwr_dwn == 255){
        do_spi_transmission_modified(config, REG_SPI_CONFIG_A, 0x81, 0x81);
        sleep_us(2000);
    } else {
        gpio_put(config->gpio_pwr_dwn, false);
        sleep_us(250);
        gpio_put(config->gpio_pwr_dwn, true);
        sleep_us(250);
        gpio_put(config->gpio_pwr_dwn, false);
        sleep_us(250);
        gpio_put(config->gpio_pwr_dwn, true);
        sleep_us(250);
        gpio_put(config->gpio_pwr_dwn, false);
        sleep_us(2000);
    }
}


bool ad4858_init(ad4858_t* config) {
    // --- Init of GPIOs
    if(config->spi_mod->init_done){
        configure_spi_module(config->spi_mod, false);
    }

    gpio_init(config->gpio_csn);
    gpio_set_dir(config->gpio_csn, GPIO_OUT);
    gpio_put(config->gpio_csn, true);

    if(config->gpio_pwr_dwn != 255){
        gpio_init(config->gpio_pwr_dwn);
        gpio_set_dir(config->gpio_pwr_dwn, GPIO_OUT);
        gpio_put(config->gpio_pwr_dwn, true);
    }

    // --- Init of the device
    ad4858_set_power_mode(config, true);
    sleep_us(500);

    ad4858_set_4_wire(config, config->use_4wire_spi);
    if(ad4858_get_product_id(config) == 0x0060){
        ad4858_do_reset(config);
        sleep_us(1000);

        ad4858_set_4_wire(config, config->use_4wire_spi);
        ad4858_enable_streaming_mode(config, false);
        ad4858_control_crc(config, config->enable_crc);
        ad4858_set_spi_output_on_sdo0(config, false);
        ad4858_enable_reference_buffer(config, config->use_ref_buffer);
        ad4858_set_reference_source(config, config->use_ext_ref);
        ad4858_enable_spi_sclk_echo(config, true);
        ad4858_enable_cmos_testpattern(config, false);
        ad4858_set_packet_size(config, AD4858_PACKETSIZE_24BIT);
        ad4858_set_oversampling_ratio(config, config->osr_ratio);
        ad4858_enable_seamless_hdr_mode(config, config->use_seamless_hdr);

        for(uint8_t i = 0; i < NUM_CHANNELS; i++){
            ad4858_set_softspan(config, i, config->softspan_level);
            ad4858_set_channel_into_sleep(config, i, false);
        }
        config->init_done = true;
    } else {
        config->init_done = false;
    }   
    return config->init_done;
};
