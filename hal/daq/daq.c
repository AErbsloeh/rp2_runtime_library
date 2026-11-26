#include "hal/daq/daq.h"
#include "hal/transport/transport.h"
#include "hal/helper/helper.h"
#include "hal/helper/crc.h"
#include <stdlib.h>
#include <string.h>
#include "pico/sync.h"


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


bool daq_init_sampling(tmr_repeat_irq_t* handler, daq_data_t* data){
    if(data == NULL){
        return false;
    }
    daq_lock_init();
    uint32_t lock_state = daq_lock_enter();
    bool init_done = false;

    if(data->buffer == NULL){
        data->buffer = calloc(1, sizeof(*data->buffer));
    }
    if(data->buffer != NULL){
        uint8_t num_blocks = 1;
        uint16_t samples_per_block = data->num_samples;
        if(data->send_mode == DAQ_MODE_SAMPLE){
            samples_per_block = 1;
        } else if(data->send_mode == DAQ_MODE_PINGPONG){
            num_blocks = 2;
        }
        const size_t bytes_per_sample = (size_t)data->num_channels * data->element_size;
        init_done = daq_buffer_init(data->buffer, num_blocks, samples_per_block, bytes_per_sample);
    }

    data->new_data = false;
    data->send_offset = 0;
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


bool daq_is_buffer_full(daq_data_t* data){
    uint32_t lock_state = daq_lock_enter();
    const bool full = daq_buffer_is_full(data->buffer);
    daq_lock_exit(lock_state);
    return full;
};


bool daq_is_buffer_empty(daq_data_t* data){
    uint32_t lock_state = daq_lock_enter();
    const bool empty = daq_buffer_is_empty(data->buffer);
    daq_lock_exit(lock_state);
    return empty;
};


uint16_t daq_get_number_bytes_per_sample(daq_data_t* data){
    return 5 + 8 + (data->num_channels * data->element_size);
}


uint16_t daq_get_number_bytes_per_batch(daq_data_t* data){
    return 5 + 2 * 8 + (data->num_channels * data->num_samples * data->element_size);
}


/*! \brief Sends the oldest complete block of the buffer as packet (sample or batch format) and releases it afterwards
    The block stays valid after the lock is released, because the producer only writes into blocks which are not ready.
    The iteration counter in the header also counts the blocks which were dropped because the buffer was full, so the host can see gaps.
    A block which does not fit to the settings of the stream (e.g. after changing the mode) is dropped, otherwise the stream is stuck.
    A packet which could not be written completely is continued with the next call (send_offset), the block stays in the buffer until the packet is complete.
    Packet: header (packet_id, iteration, first timestamp, batch only: last timestamp), data of the block, CRC16 (2 bytes), tail
    \param data       Pointer to the DAQ data structure
    \return           true if a packet was sent completely, false otherwise
*/
static bool daq_send_block(daq_data_t* data){
    if((data == NULL) || (data->buffer == NULL)){
        return false;
    }

    uint32_t lock_state = daq_lock_enter();
    const daq_buffer_block_t *block = daq_buffer_get_read_block(data->buffer);
    if((block != NULL) && (data->send_offset == 0) && (data->buffer->samples_per_block > 0)){
        uint32_t lost_blocks = data->buffer->num_dropped / data->buffer->samples_per_block;
        if(lost_blocks > 0){
            data->buffer->num_dropped -= lost_blocks * data->buffer->samples_per_block;
            data->iteration = (uint8_t)(data->iteration + lost_blocks);
        }
    }
    daq_lock_exit(lock_state);
    if(block == NULL){
        return false;
    }

    const bool use_batch = (data->send_mode != DAQ_MODE_SAMPLE);
    const size_t num_samples = use_batch ? data->num_samples : 1;
    const size_t payload_size = num_samples * data->num_channels * data->element_size;
    const size_t frame_size = use_batch ? daq_get_number_bytes_per_batch(data) : daq_get_number_bytes_per_sample(data);
    const size_t offset_data = use_batch ? 18 : 10;

    if((block->sample_count != num_samples) || (payload_size > block->capacity_bytes)){
        data->send_offset = 0;
        lock_state = daq_lock_enter();
        daq_buffer_release_read(data->buffer);
        data->new_data = (daq_buffer_get_read_block(data->buffer) != NULL);
        daq_lock_exit(lock_state);
        __sev();
        return false;
    }
    if(data->send_offset >= frame_size){
        data->send_offset = 0;
    }

    char buffer[frame_size];

    buffer[0] = data->packet_id;
    buffer[1] = (uint8_t)(data->iteration + 1u);
    uint64_t runtime = block->timestamp_first;
    for(uint8_t idx = 0; idx < sizeof(uint64_t); idx++){
        buffer[2 + idx] = (uint8_t)runtime;
        runtime >>= 8;
    }
    if(use_batch){
        runtime = block->timestamp_last;
        for(uint8_t idx = 0; idx < sizeof(uint64_t); idx++){
            buffer[10 + idx] = (uint8_t)runtime;
            runtime >>= 8;
        }
    }

    memcpy(&buffer[offset_data], block->data, payload_size);

    uint16_t crc = crc16_ccitt((const uint8_t*)buffer, frame_size - 3);
    buffer[frame_size - 3] = (uint8_t)(crc & 0xFF);
    buffer[frame_size - 2] = (uint8_t)(crc >> 8);
    buffer[frame_size - 1] = (uint8_t)(data->packet_tail);

    const size_t remaining = frame_size - data->send_offset;
    const size_t written = transport_write(&buffer[data->send_offset], remaining);
    if(written < remaining){
        data->send_offset += written;
        return false;
    }
    data->send_offset = 0;
    data->runtime_first = block->timestamp_first;
    data->runtime_last = block->timestamp_last;
    data->iteration++;

    lock_state = daq_lock_enter();
    daq_buffer_release_read(data->buffer);
    data->new_data = (daq_buffer_get_read_block(data->buffer) != NULL);
    daq_lock_exit(lock_state);
    __sev();
    return true;
}


bool daq_check_send_data(daq_data_t* data){
    return daq_send_block(data);
};


void daq_send_data(daq_data_t* data){
    daq_send_block(data);
};


bool daq_irq_process(daq_data_t* config, void* data){
    uint32_t lock_state = daq_lock_enter();
    if(config->buffer != NULL){
        daq_buffer_push_sample(config->buffer, data, get_runtime_us());
        config->new_data = (daq_buffer_get_read_block(config->buffer) != NULL);
    }
    daq_lock_exit(lock_state);
    return true;
}
