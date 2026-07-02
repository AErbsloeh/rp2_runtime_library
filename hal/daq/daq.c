#include "hal/daq/daq.h"
#include "hal/transport/transport.h"
#include "hal/helper/helper.h"
#include "hal/helper/crc.h"
#include <stdlib.h>

// TODO: Check structure for double_buffer
//==================== CALLABLE FUNCS ====================//
bool daq_init_sampling(tmr_repeat_irq_t* handler, daq_data_t* data){
    size_t num_samples = 0;
    if(data->send_mode == DAQ_MODE_SAMPLE) {
        num_samples = 1;
    } else {
        num_samples = data->num_samples;
    };

    data->data0->element_size = data->element_size;
    data->data0->length = num_samples * data->num_channels;
    fifo_init(data->data0);

    if(data->send_mode == DAQ_MODE_BUFFER_DOUBLE)  {
        data->data1->element_size = data->element_size;
        data->data1->length = num_samples * data->num_channels;
        fifo_init(data->data1);
    }

    data->new_data = false;
    data->first_buffer_full = false;
    return init_timer_irq(handler);
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
    if(data->send_mode == DAQ_MODE_BUFFER_DOUBLE) {
        if(data->first_buffer_full) {
            return fifo_push(data->data1, new_data_in);
        } else {
            return fifo_push(data->data0, new_data_in);
        }
    } else {
        return fifo_push(data->data0, new_data_in);
    }
};


bool daq_pop_data_from_fifo(daq_data_t* data, void* data_out){
    if(data->send_mode == DAQ_MODE_BUFFER_DOUBLE) {
        if(data->first_buffer_full) {
            return fifo_pop(data->data1, data_out);
        } else {
            return fifo_pop(data->data0, data_out);
        }
    } else {
        return fifo_pop(data->data0, data_out);
    }
};


bool daq_is_fifo_full(daq_data_t* data){
    if(data->send_mode == DAQ_MODE_BUFFER_DOUBLE) {
        if(data->first_buffer_full){
            return fifo_is_full(data->data1);
        } else {
            return fifo_is_full(data->data0);
        }
    } else {
        return fifo_is_full(data->data0);
    }
};


bool daq_is_empty_fifo(daq_data_t* data){
    if(data->send_mode == DAQ_MODE_BUFFER_DOUBLE) {
        if(data->first_buffer_full){
            return fifo_is_empty(data->data1);
        } else {
            return fifo_is_empty(data->data0);
        }
    } else {
        return fifo_is_empty(data->data0);
    }
};


//calculate packet size: header (3 bytes) + timestamp (8 bytes) + data (num_channels * element_size) + CRC (2 bytes)
uint16_t daq_get_number_bytes_per_sample(daq_data_t* data){
    return 5 + 8 + (data->num_channels * data->element_size);
}


void daq_send_data_sample(daq_data_t* data){
    const size_t data_format = data->element_size;
    const size_t frame_size = daq_get_number_bytes_per_sample(data);
    char buffer[frame_size];

    // Header Frame
    buffer[0] = data->packet_id;
    buffer[1] = data->iteration;
    uint64_t runtime = data->runtime_first;
    for(uint8_t idx = 0; idx < sizeof(uint64_t); idx++){
        buffer[2+idx] = (uint8_t)runtime;
        runtime >>= 8;
    };
    // Data Frame
    uint8_t data_process[data_format];
    size_t smp = 0;
    while(!daq_is_empty_fifo(data)){
        daq_pop_data_from_fifo(data, data_process);
        for(size_t idx = 0; idx < data_format; idx++){
            buffer[10 + (smp * data_format) + idx] = data_process[idx];
        }
        smp++;
    };
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

    // Header Frame
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
    // Data Frame
    uint8_t data_process[data_format];
    size_t smp = 0;
    while(!daq_is_empty_fifo(data)){
        daq_pop_data_from_fifo(data, data_process);
        for(size_t idx = 0; idx < data_format; idx++){
            buffer[18 + (smp * data_format) + idx] = data_process[idx];
        }
        smp++;
    };
    // End Frame
    uint16_t crc = crc16_ccitt((const uint8_t*)buffer, frame_size - 3);
    buffer[frame_size-3] = (uint8_t)(crc & 0xFF);
    buffer[frame_size-2] = (uint8_t)(crc >> 8);
    buffer[frame_size-1] = (uint8_t)(data->packet_tail);
    transport_write(buffer, sizeof(buffer));
};


bool daq_check_send_data(daq_data_t* data){
    if(data->new_data){
        data->new_data = false;  
        daq_send_data(data);
        return true;
    } else {
        return false;
    }
    
};


void daq_send_data(daq_data_t* data){
    data->iteration++;
    if(data->send_mode > DAQ_MODE_SAMPLE){
        daq_send_data_batch(data);
    } else {
        daq_send_data_sample(data);
    }
};


bool daq_irq_process(daq_data_t* config, void* data){
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

    if(daq_is_fifo_full(config)){
        config->new_data = true;
        if(config->send_mode == DAQ_MODE_BUFFER_DOUBLE)
            config->first_buffer_full = !config->first_buffer_full;
    } else {
        config->new_data = false;
    }
    return true;
}
