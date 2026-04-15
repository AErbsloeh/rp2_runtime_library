#include "hal/daq/daq.h"
#include "hal/usb/usb.h"
#include "hal/helper/helper.h"
#include <stdlib.h>


//==================== CALLABLE FUNCS ====================//
bool daq_init_sampling(tmr_repeat_irq_t* handler, daq_data_t* data){
    data->data->length = data->num_samples * data->num_channels;
    data->send_batch = (data->num_samples > 1);
    data->new_data = false;

    fifo_init(data->data);
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
    return fifo_push(data->data, new_data_in);
};


bool daq_pop_data_from_fifo(daq_data_t* data, void* data_out){
    return fifo_pop(data->data, data_out);
};


bool daq_is_fifo_full(daq_data_t* data){
    return fifo_is_full(data->data);
};


bool daq_is_empty_fifo(daq_data_t* data){
    return fifo_is_empty(data->data);
};


uint16_t daq_get_number_bytes_per_sample(daq_data_t* data){
    return 3 + 8 + (data->num_channels * data->data->element_size);
}


void daq_send_data_sample(daq_data_t* data){
    const size_t data_format = data->data->element_size;
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
    uint16_t data_process = 0;
    size_t smp = 0;
    while(!daq_is_empty_fifo(data)){
        daq_pop_data_from_fifo(data, &data_process);
        for(uint8_t idx = 0; idx < data_format; idx++){
            buffer[10 + (smp * data_format) + idx] = (uint8_t)(data_process >> (8*idx));
        };
        smp++;
    };
    // End Frame
    buffer[frame_size-1] = 0xFF;
    usb_send_bytes(buffer, sizeof(buffer));
};


uint16_t daq_get_number_bytes_per_batch(daq_data_t* data){
    return 3 + 2*8 + (data->data->length * data->data->element_size);
}


void daq_send_data_batch(daq_data_t* data){
    const size_t data_format = data->data->element_size;
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
    uint16_t data_process = 0;
    size_t smp = 0;
    while(!daq_is_empty_fifo(data)){
        daq_pop_data_from_fifo(data, &data_process);
        for(uint8_t idx = 0; idx < data_format; idx++){
            buffer[18 + (smp * data_format) + idx] = (uint8_t)(data_process >> (8*idx));
        };
        smp++;
    };
    // End Frame
    buffer[frame_size-1] = 0xFF;
    usb_send_bytes(buffer, sizeof(buffer));
};


bool daq_check_send_data(daq_data_t* data){
    if(data->new_data){
        data->new_data = false;  
        daq_send_data_usb(data);
        return true;
    } else {
        return false;
    }
    
};


void daq_send_data_usb(daq_data_t* data){
    data->iteration ++;
    if(data->send_batch){
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
        void* element = ptr + (idx * config->data->element_size);
        daq_push_data_to_fifo(config, element);
    }    
    config->new_data = (config->send_batch) ? daq_is_fifo_full(config) : true;
    return true;
}
