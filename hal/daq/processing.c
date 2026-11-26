#include "hal/daq/processing.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include <stdlib.h>
#include <string.h>


static const processing_config_t *config_active = NULL;
static bool core1_started = false;
static queue_t queue_in[PROCESSING_NUM_SOURCES];
static uint8_t *buffer_in = NULL;
static uint8_t *buffer_out = NULL;
static uint8_t *buffer_sample = NULL;
static size_t num_samples_collected = 0;
static volatile uint32_t num_dropped_in[PROCESSING_NUM_SOURCES];


size_t processing_get_num_bytes(const processing_shape_t *shape){
    return (size_t)shape->num_channels * shape->num_samples * shape->element_size;
}


static bool shape_is_valid(const processing_shape_t *shape){
    return (shape->num_channels > 0) && (shape->num_samples > 0) && (shape->element_size > 0);
}


static bool daq_matches_shape(const daq_data_t *daq, const processing_shape_t *shape){
    if((daq == NULL) || (daq->num_channels != shape->num_channels) || (daq->element_size != shape->element_size)){
        return false;
    }
    // The stream collects whole blocks: a block has to fit into the FIFO of the stream (one sample per packet or a multiple of the block)
    if(daq->send_mode == DAQ_MODE_SAMPLE){
        return shape->num_samples == 1;
    }
    return (daq->num_samples % shape->num_samples) == 0;
}


bool processing_init(const processing_config_t *config){
    if(core1_started){
        return false;
    }
    config_active = NULL;
    if((config == NULL) || !shape_is_valid(&config->in) || !shape_is_valid(&config->out) || (config->queue_depth == 0) || (config->func == NULL) || !daq_matches_shape(config->daq_out, &config->out)){
        return false;
    }

    const size_t num_bytes_in = processing_get_num_bytes(&config->in);
    const size_t num_bytes_out = processing_get_num_bytes(&config->out);
    free(buffer_in);
    free(buffer_out);
    free(buffer_sample);
    buffer_in = malloc(num_bytes_in);
    buffer_out = malloc(num_bytes_out);
    buffer_sample = malloc(num_bytes_in);
    if((buffer_in == NULL) || (buffer_out == NULL) || (buffer_sample == NULL)){
        return false;
    }

    for(uint8_t src = 0; src < PROCESSING_NUM_SOURCES; src++){
        queue_init(&queue_in[src], num_bytes_in, config->queue_depth);
        num_dropped_in[src] = 0;
    }
    num_samples_collected = 0;
    config_active = config;
    return true;
}


static void core1_entry(void){
    const size_t num_bytes_sample = (size_t)config_active->out.num_channels * config_active->out.element_size;
    uint8_t first_src = 0;
    while(true){
        bool idle = true;
        // Only process a block if the FIFO of the DAQ stream is not full, so the result always fits into it
        if(!daq_is_fifo_full(config_active->daq_out)){
            for(uint8_t cnt = 0; cnt < PROCESSING_NUM_SOURCES; cnt++){
                const uint8_t src = (first_src + cnt) % PROCESSING_NUM_SOURCES;
                if(!queue_try_remove(&queue_in[src], buffer_in)){
                    continue;
                }
                idle = false;
                first_src = (src + 1) % PROCESSING_NUM_SOURCES;
                if(config_active->func(config_active, buffer_in, buffer_out)){
                    for(uint16_t smp = 0; smp < config_active->out.num_samples; smp++){
                        daq_irq_process(config_active->daq_out, &buffer_out[smp * num_bytes_sample]);
                    }
                    break;
                }
            }
        }
        if(idle){
            __wfe();
        }
    }
}


bool processing_start(void){
    if((config_active == NULL) || core1_started){
        return false;
    }
    core1_started = true;
    multicore_launch_core1(core1_entry);
    return true;
}


const processing_config_t* processing_get_config(void){
    return config_active;
}


bool processing_submit(processing_source_t source, const uint8_t *in){
    if((config_active == NULL) || (source >= PROCESSING_NUM_SOURCES)){
        return false;
    }
    if(!queue_try_add(&queue_in[source], in)){
        num_dropped_in[source]++;
        return false;
    }
    return true;
}


bool processing_push_sample(const uint8_t *sample, size_t length){
    if(config_active == NULL){
        return false;
    }
    const size_t num_bytes_sample = (size_t)config_active->in.num_channels * config_active->in.element_size;
    if(length != num_bytes_sample){
        return false;
    }

    memcpy(&buffer_sample[num_samples_collected * num_bytes_sample], sample, num_bytes_sample);
    num_samples_collected++;
    if(num_samples_collected < config_active->in.num_samples){
        return true;
    }
    num_samples_collected = 0;
    return processing_submit(PROCESSING_SOURCE_SENSOR, buffer_sample);
}


uint32_t processing_get_num_dropped(void){
    uint32_t num_dropped = 0;
    for(uint8_t src = 0; src < PROCESSING_NUM_SOURCES; src++){
        num_dropped += num_dropped_in[src];
    }
    return num_dropped;
}
