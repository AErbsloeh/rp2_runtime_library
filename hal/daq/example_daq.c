#include "pico/stdlib.h"
#include "hal/daq/daq.h"


int main() {
    stdio_init_all();

    // --- DAQ Sampling
    fifo_t daq_fifo = {
        .element_size = sizeof(uint16_t)
    };
    daq_data_t daq_sample_data = {
        .packet_id = 0xA0,
        .iteration = 0,
        .runtime_first = 0,
        .runtime_last = 0,
        .num_channels = 2,
        .num_samples = 64,
        .data = &daq_fifo,
        .send_batch = false,
        .new_data = false
    };
    uint16_t daq_data[2] = {0, 0};
    bool irq_tmr_daq0(repeating_timer_t *rt){
        daq_data[0] += 8;
        daq_data[1] -= 8;
        return daq_irq_process(&daq_sample_data, daq_data);    
    };
    repeating_timer_t tmr_daq0;
    tmr_repeat_irq_t tmr_daq0_hndl = {
        .timer = &tmr_daq0,
        .irq_number = 0,
        .period_us = -250000,
        .alarm_done = false,
        .enable_state = false,
        .init_done = false,
        .func_irq = irq_tmr_daq0
    };

    fifo_init(&my_fifo);
    uint8_t value = 0;
    uint8_t out;

    daq_start_sampling(&daq_sample_data);

    while (true) {
        tight_loop_contents();
        daq_check_send_data(&daq_sample_data);
    }
}