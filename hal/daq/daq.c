#include "hal/daq/daq.h"
#include "hal/usb/usb.h"
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


bool daq_check_send_data(daq_data_t* data){
    if(daq_sample_data.send_batch){
        if(daq_is_fifo_full(&daq_sample_data)){
            daq_sample_data.iteration ++;
            daq_send_data_usb(&daq_sample_data, daq_sample_data.data->length);
            return true;
        };    
    } else {
        if(daq_sample_data.new_data){
            daq_sample_data.new_data = false;  
            daq_sample_data.iteration ++;
            daq_send_data_usb(&daq_sample_data, 2);
            return true;
        };
    }
    return false;
};


void daq_send_data_usb(daq_data_t* data, size_t num_samples){
    const size_t data_format = data->data->element_size;
    const size_t frame_size = 11 + (num_samples * data_format);
    char buffer[frame_size];

    // Header Frame
    buffer[0] = data->packet_id;
    buffer[1] = data->iteration;
    uint64_t runtime = data->runtime;
    for(uint8_t idx = 0; idx < sizeof(uint64_t); idx++){
        buffer[2+idx] = (uint8_t)runtime;
        runtime >>= 8;
    };
    // Data Frame
    uint16_t data_process = 0;
    for(size_t smp = 0; smp < num_samples; smp++){
        daq_pop_data_from_fifo(data, &data_process);
        for(uint8_t idx = 0; idx < data_format; idx++){
            buffer[10 + (smp * data_format) + idx] = (uint8_t)(data_process >> (8*idx));
        };
    };
    // End Frame
    buffer[frame_size-1] = 0xFF;
    usb_send_bytes(buffer, sizeof(buffer));
};
