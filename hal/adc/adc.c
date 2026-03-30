#include "hal/adc/adc.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include <stdlib.h>
#include <stdio.h>


// ======================= INTERNAL VARIABLES AND FUNCS ========================
static int rp2_dma_channel = 0;
static dma_channel_config rp2_dma_cfg;

// --- Data buffer
volatile uint64_t timestamp = 0;
volatile uint16_t* buffer_a;
volatile uint16_t* buffer_b;

volatile size_t buffer_idx = 0;
volatile bool use_buffer_a = true;
volatile bool buffer_a_rdy = false;
volatile bool buffer_b_rdy = false;


uint64_t get_timestamp(void){
    absolute_time_t now = get_absolute_time();
    return to_us_since_boot(now);
}


// --- IRQ handler
void rp2_adc_dma_handler(void){
    dma_hw->ints0 = 1u << rp2_dma_channel;
    timestamp = get_timestamp();

    if (use_buffer_a) {
        buffer_a_rdy = true;
        use_buffer_a = false;
        dma_channel_set_write_addr(rp2_dma_channel, buffer_b, true);
    } else {
        buffer_b_rdy = true;
        use_buffer_a = true;
        dma_channel_set_write_addr(rp2_dma_channel, buffer_a, true);
    }
}


// =============================== GLOBAL FUNCS ===============================
uint8_t get_gpio_rp2_adc_channel(uint8_t channel){
    switch(channel){
        case RP2_ADC_CH0:
            return 26;
        case RP2_ADC_CH1:
            return 27;
        case RP2_ADC_CH2:
            return 28;
        case RP2_ADC_CH3:
            return 29;
        case RP2_ADC_TEMP:
            return 0;
        default:
            return 0;
    }
}


uint8_t rp2_adc_get_current_channel(void){
    return adc_hw->cs & ADC_CS_AINSEL_BITS;
}


bool rp2_adc_init_dma(rp2_adc_t* config){
    adc_fifo_setup(
        true,   // Activating FIFO buffer
        true,   // Activating DMA request
        1,      // Catch every for samples
        false,  // No error flag
        false   // no shift (getting 12-bit data)
    );

    float clkdiv = 48000000.0f / ((float)config->sampling_rate);
    adc_set_clkdiv(clkdiv);

    // DMA Setup
    rp2_dma_channel = dma_claim_unused_channel(true);
    rp2_dma_cfg = dma_channel_get_default_config(rp2_dma_channel);

    channel_config_set_transfer_data_size(&rp2_dma_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&rp2_dma_cfg, false);
    channel_config_set_write_increment(&rp2_dma_cfg, true);
    channel_config_set_dreq(&rp2_dma_cfg, DREQ_ADC);
    dma_channel_configure(
        rp2_dma_channel,
        &rp2_dma_cfg,
        buffer_a,           // Data Destination
        &adc_hw->fifo,      // Data Source
        config->buffersize, // Buffer Size
        false               // Start DMA controller
    );

    dma_channel_set_irq0_enabled(rp2_dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, rp2_adc_dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    config->init_done = true;
    return config->init_done;
}


bool rp2_adc_init(rp2_adc_t* config){
    if(config->init_done)
        return true;

    adc_init();
    rp2_adc_change_channel(config, config->adc_channel);

    if(config->buffersize > 1){
        buffer_a = malloc(config->buffersize * sizeof(uint16_t));
        buffer_b = malloc(config->buffersize * sizeof(uint16_t));
    }
    if(config->use_dma){
        return rp2_adc_init_dma(config);
    } else {
        config->init_done = true;
        return config->init_done;
    }    
}


bool rp2_adc_change_channel(rp2_adc_t* config, uint8_t new_channel){
    if(new_channel == RP2_ADC_TEMP)
        adc_set_temp_sensor_enabled(true); 
    else{
        adc_set_temp_sensor_enabled(false); 
        adc_gpio_init(get_gpio_rp2_adc_channel(new_channel));
    }
    adc_select_input(new_channel);
    config->adc_channel = new_channel;
    return true;
}


uint16_t rp2_adc_read_raw(rp2_adc_t* config){
    if(!config->init_done)
        return 65535;

    return adc_read();
}


bool rp2_adc_start_buffer(rp2_adc_t* config){
    if(!config->init_done)
        return false;
    
    buffer_idx = 0;
    use_buffer_a = true;
    buffer_a_rdy = false;
    buffer_b_rdy = false;
    for (size_t idx = 0; idx < config->buffersize; idx++) {
        buffer_a[buffer_idx] = 0;
        buffer_b[buffer_idx] = 0;
    }

    if(config->use_dma){
        dma_channel_start(rp2_dma_channel);
        adc_run(true);
    };
    return true;
}


bool rp2_adc_stop_buffer(rp2_adc_t* config){
    if(!config->init_done)
        return false;
    
    if(config->use_dma){
        dma_channel_abort(rp2_dma_channel);
        adc_run(false);
    }
    return true;
}


bool rp2_adc_read_buffer_polling(rp2_adc_t* config){
    if(!config->init_done)
        return false;
    
    buffer_a[buffer_idx] = adc_read();
    timestamp = get_timestamp();

    if(buffer_idx == config->buffersize-1){
        buffer_a_rdy = true;
        buffer_idx = 0;
        return true;
    } else {
        buffer_a_rdy = false;
        buffer_idx++;
        return false;
    }
}


volatile uint16_t* rp2_adc_get_buffer(rp2_adc_t* config) {
    if(!config->init_done)
        return NULL;

    if(buffer_a_rdy){
        buffer_a_rdy = false;
        return buffer_a;
    }
    else if(buffer_b_rdy){
        buffer_b_rdy = false;
        return buffer_b;
    } else {
        return NULL;
    };
}


size_t rp2_adc_get_buffersize(void){
    return (use_buffer_a) ? sizeof(buffer_a) : sizeof(buffer_b);
}


uint64_t rp2_adc_get_timestamp(void){
    return timestamp;
}