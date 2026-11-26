#include "pico/stdlib.h"
#include "hal/daq/daq.h"

#define DAQ_EXAMPLE_NUM_CHANNELS 2u
#define DAQ_EXAMPLE_NUM_SAMPLES 64u

int main() {
    stdio_init_all();

    // --- DAQ sampling (DAQ_MODE_PINGPONG: two blocks, written and sent alternately)

    // --- DAQ sampling
    daq_data_t daq_sample_data = {
        .packet_id = 0xA0,
        .iteration = 0,
        .runtime_first = 0,
        .runtime_last = 0,
        .num_channels = DAQ_EXAMPLE_NUM_CHANNELS,
        .num_samples = DAQ_EXAMPLE_NUM_SAMPLES,
        .is_signed = false,
        .element_size = sizeof(uint16_t),
        .send_mode = DAQ_MODE_PINGPONG,
        .new_data = false,
        .buffer = NULL
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

    uint8_t value = 0;
    uint8_t out;

    if (!daq_init_sampling(&tmr_daq0_hndl, &daq_sample_data)) {
        return 1;
    }
    daq_start_sampling(&tmr_daq0_hndl);

    while (true) {
        tight_loop_contents();
        daq_check_send_data(&daq_sample_data);
    }
}