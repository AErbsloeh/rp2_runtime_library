#include "hal/daq/daq.h"
#include "hal/transport/transport.h"
#include "hal/helper/helper.h"
#include "hal/helper/crc.h"
#include <stdlib.h>
#include <string.h>
#include "pico/sync.h"


// Lock for the DAQ streams: the FIFO of a stream can be filled by another core than the one which sends it
static spin_lock_t *daq_lock = NULL;


static void daq_lock_init(void){
    if(daq_lock == NULL){
        daq_lock = spin_lock_init(spin_lock_claim_unused(true));
    }
}


static inline uint32_t daq_lock_enter(void){
    return (daq_lock != NULL) ? spin_lock_blocking(daq_lock) : 0;
}


static inline void daq_lock_exit(uint32_t state){
    if(daq_lock != NULL){
        spin_unlock(daq_lock, state);
    }
}


//==================== CALLABLE FUNCS ====================//
bool daq_init_sampling(tmr_repeat_irq_t* handler, daq_data_t* data){
    if(data == NULL){
        return false;
    }
    daq_lock_init();
    uint32_t lock_state = daq_lock_enter();
    bool init_done = true;

    if(data->send_mode == DAQ_MODE_BUFFER_DOUBLE){
        if(data->ping_pong == NULL){
            data->ping_pong = calloc(1, sizeof(*data->ping_pong));
        }
        if(data->ping_pong != NULL){
            const size_t bytes_per_sample = (size_t)data->num_channels * data->element_size;
            init_done = ping_pong_buffer_init(data->ping_pong, data->num_samples, bytes_per_sample);
        } else {
            init_done = false;
        }
    } else {
        if(data->fifo == NULL){
            data->fifo = calloc(1, sizeof(*data->fifo));
        }
        if(data->fifo != NULL){
            const size_t num_samples = (data->send_mode == DAQ_MODE_SAMPLE) ? 1 : data->num_samples;
            if(data->fifo->queue.data != NULL){
                queue_free(&data->fifo->queue);
            }
            data->fifo->element_size = data->element_size;
            data->fifo->length = num_samples * data->num_channels;
            fifo_init(data->fifo);
        } else {
            init_done = false;
        }
    }

    data->new_data = false;
    daq_lock_exit(lock_state);
    return init_done && init_timer_irq(handler);
};


bool daq_update_sampling_rate(tmr_repeat_irq_t* handler, int64_t new_rate_us){
    bool daq_enabled = handler->enable_state;
    if(daq_enabled){
        disable_repeat_timer_irq(handler);
    };
    
    handler->period_us = new_rate_us;
    if(daq_enabled){
        enable_repeat_timer_irq(handler);
    }
    return daq_enabled;
};


bool daq_start_sampling(tmr_repeat_irq_t* handler){
    return enable_repeat_timer_irq(handler);
};


bool daq_stop_sampling(tmr_repeat_irq_t* handler){
    return disable_repeat_timer_irq(handler);
};


bool daq_push_data_to_fifo(daq_data_t* data, void* new_data_in){
    if(data->fifo == NULL){
        return false;
    }
    return fifo_push(data->fifo, new_data_in);
};


bool daq_pop_data_from_fifo(daq_data_t* data, void* data_out){
    if(data->fifo == NULL){
        return false;
    }
    return fifo_pop(data->fifo, data_out);
};


bool daq_is_fifo_full(daq_data_t* data){
    if(data->send_mode == DAQ_MODE_BUFFER_DOUBLE){
        // Full if the block for writing is still waiting for being sent (both blocks are used)
        if(data->ping_pong == NULL){
            return true;
        }
        return data->ping_pong->blocks[data->ping_pong->write_index & (PING_PONG_NUM_BLOCKS - 1u)].ready;
    }
    if(data->fifo == NULL){
        return true;
    }
    return fifo_is_full(data->fifo);
};


bool daq_is_empty_fifo(daq_data_t* data){
    if(data->send_mode == DAQ_MODE_BUFFER_DOUBLE){
        if(data->ping_pong == NULL){
            return true;
        }
        const ping_pong_block_t *block_write = &data->ping_pong->blocks[data->ping_pong->write_index & (PING_PONG_NUM_BLOCKS - 1u)];
        return (ping_pong_buffer_get_read_block(data->ping_pong) == NULL) && (block_write->sample_count == 0);
    }
    if(data->fifo == NULL){
        return true;
    }
    return fifo_is_empty(data->fifo);
};


//calculate packet size: header (3 bytes) + timestamp (8 bytes) + data (num_channels * element_size) + CRC (2 bytes)
uint16_t daq_get_number_bytes_per_sample(daq_data_t* data){
    return 5 + 8 + (data->num_channels * data->element_size);
}


void daq_send_data_sample(daq_data_t* data){
    const size_t data_format = data->element_size;
    const size_t frame_size = daq_get_number_bytes_per_sample(data);
    char buffer[frame_size];
    const size_t max_elements = data->num_channels;

    // Header Frame and Data Frame (FIFO is locked while it is emptied)
    uint32_t lock_state = daq_lock_enter();
    buffer[0] = data->packet_id;
    buffer[1] = data->iteration;
    uint64_t runtime = data->runtime_first;
    for(uint8_t idx = 0; idx < sizeof(uint64_t); idx++){
        buffer[2+idx] = (uint8_t)runtime;
        runtime >>= 8;
    };
    uint8_t data_process[data_format];
    size_t smp = 0;
    while((smp < max_elements) && !daq_is_empty_fifo(data)){
        daq_pop_data_from_fifo(data, data_process);
        for(size_t idx = 0; idx < data_format; idx++){
            buffer[10 + (smp * data_format) + idx] = data_process[idx];
        }
        smp++;
    };
    daq_lock_exit(lock_state);
    // End Frame (CRC + tail)
    uint16_t crc = crc16_ccitt((const uint8_t*)buffer, frame_size - 3);
    buffer[frame_size-3] = (uint8_t)(crc & 0xFF);
    buffer[frame_size-2] = (uint8_t)(crc >> 8);
    buffer[frame_size-1] = (uint8_t)(data->packet_tail);
    transport_write(buffer, sizeof(buffer));
};


//calculate packet size: header (3 bytes) + timestamp (8 bytes) + data (num_channels * num_samples * element_size) + CRC (2 bytes)
uint16_t daq_get_number_bytes_per_batch(daq_data_t* data){
    return 5 + 2 * 8 + (data->num_channels * data->num_samples * data->element_size);
}


void daq_send_data_batch(daq_data_t* data){
    const size_t data_format = data->element_size;
    const size_t frame_size = daq_get_number_bytes_per_batch(data);
    char buffer[frame_size];
    const size_t max_elements = (size_t)data->num_channels * data->num_samples;

    // Header Frame and Data Frame (FIFO is locked while it is emptied)
    uint32_t lock_state = daq_lock_enter();
    buffer[0] = data->packet_id;
    buffer[1] = data->iteration;
    uint64_t runtime = data->runtime_first;
    for(uint8_t idx = 0; idx < sizeof(uint64_t); idx++){
        buffer[2+idx] = (uint8_t)runtime;
        runtime >>= 8;
    };
    runtime = data->runtime_last;
    for(uint8_t idx = 0; idx < sizeof(uint64_t); idx++){
        buffer[10+idx] = (uint8_t)runtime;
        runtime >>= 8;
    };
    uint8_t data_process[data_format];
    size_t smp = 0;
    while((smp < max_elements) && !daq_is_empty_fifo(data)){
        daq_pop_data_from_fifo(data, data_process);
        for(size_t idx = 0; idx < data_format; idx++){
            buffer[18 + (smp * data_format) + idx] = data_process[idx];
        }
        smp++;
    };
    daq_lock_exit(lock_state);
    // End Frame
    uint16_t crc = crc16_ccitt((const uint8_t*)buffer, frame_size - 3);
    buffer[frame_size-3] = (uint8_t)(crc & 0xFF);
    buffer[frame_size-2] = (uint8_t)(crc >> 8);
    buffer[frame_size-1] = (uint8_t)(data->packet_tail);
    transport_write(buffer, sizeof(buffer));
};


static bool daq_send_data_ping_pong(daq_data_t *data){
    if((data == NULL) || (data->ping_pong == NULL)){
        return false;
    }
    const ping_pong_block_t *block = ping_pong_buffer_get_read_block(data->ping_pong);
    if((block == NULL) || (block->data == NULL) || (block->sample_count != data->num_samples)){
        return false;
    }

    const size_t frame_size = daq_get_number_bytes_per_batch(data);
    const size_t payload_size = (size_t)block->sample_count * data->num_channels * data->element_size;
    if(payload_size > block->capacity_bytes){
        return false;
    }
    char buffer[frame_size];

    // Header Frame
    buffer[0] = data->packet_id;
    buffer[1] = (uint8_t)(data->iteration + 1u);
    uint64_t runtime = block->timestamp_first;
    for(uint8_t idx = 0; idx < sizeof(uint64_t); idx++){
        buffer[2 + idx] = (uint8_t)runtime;
        runtime >>= 8;
    }
    runtime = block->timestamp_last;
    for(uint8_t idx = 0; idx < sizeof(uint64_t); idx++){
        buffer[10 + idx] = (uint8_t)runtime;
        runtime >>= 8;
    }

    // Data Frame
    memcpy(&buffer[18], block->data, payload_size);

    // End Frame
    uint16_t crc = crc16_ccitt((const uint8_t*)buffer, frame_size - 3);
    buffer[frame_size - 3] = (uint8_t)(crc & 0xFF);
    buffer[frame_size - 2] = (uint8_t)(crc >> 8);
    buffer[frame_size - 1] = (uint8_t)data->packet_tail;

    if(transport_write(buffer, frame_size) != frame_size){
        return false;
    }
    if(!ping_pong_buffer_release_read(data->ping_pong)){
        return false;
    }
    data->iteration++;
    return true;
}


bool daq_check_send_data(daq_data_t* data){
    if(data == NULL){
        return false;
    }

    if(data->send_mode == DAQ_MODE_BUFFER_DOUBLE){
        return daq_send_data_ping_pong(data);
    }

    uint32_t lock_state = daq_lock_enter();
    const bool ready = data->new_data;
    if(ready){
        data->new_data = false;
    }
    daq_lock_exit(lock_state);
    if(ready){
        daq_send_data(data);
        return true;
    } else {
        return false;
    }
};


void daq_send_data(daq_data_t* data){
    if(data->send_mode == DAQ_MODE_BUFFER_DOUBLE){
        daq_send_data_ping_pong(data);
        return;
    }
    data->iteration++;
    if(data->send_mode > DAQ_MODE_SAMPLE){
        daq_send_data_batch(data);
    } else {
        daq_send_data_sample(data);
    }
};


bool daq_irq_process(daq_data_t* config, void* data){
    uint32_t lock_state = daq_lock_enter();

    if(config->send_mode == DAQ_MODE_BUFFER_DOUBLE){
        if(config->ping_pong != NULL){
            (void)ping_pong_buffer_push_sample(config->ping_pong, data, get_runtime_ms());
        }
        daq_lock_exit(lock_state);
        return true;
    }

    if(daq_is_empty_fifo(config)){
        config->runtime_first = get_runtime_ms();
    } else {
        config->runtime_last = get_runtime_ms();
    }

    uint8_t* ptr = (uint8_t*)data;    
    for(size_t idx=0; idx < config->num_channels; idx++){
        void* element = ptr + (idx * config->element_size);
        daq_push_data_to_fifo(config, element);
    }

    config->new_data = daq_is_fifo_full(config);
    daq_lock_exit(lock_state);
    return true;
}
