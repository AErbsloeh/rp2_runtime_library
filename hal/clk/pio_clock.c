#include "hal/clk/pio_clock.h"
#include "pio_clock.pio.h"
#include "hardware/clocks.h"


#define PIO_CLK_EXTRA_CYCLES 3u


bool clk_generation_pio_init(PIO pio, uint pin, uint freq_hz){
    if(freq_hz == 0){
        return false;
    }
    const uint64_t half_period_cycles = clock_get_hz(clk_sys) / (2ull * freq_hz);
    if(half_period_cycles < PIO_CLK_EXTRA_CYCLES){
        return false;
    }
    if(!pio_can_add_program(pio, &blink_program)){
        return false;
    }
    int sm = pio_claim_unused_sm(pio, false);
    if(sm < 0){
        return false;
    }
    uint offset = pio_add_program(pio, &blink_program);

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_pull_down(pin);
    gpio_put(pin, false);

    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    pio_sm_config c = blink_program_get_default_config(offset);
    sm_config_set_set_pins(&c, pin, 1);
    pio_sm_init(pio, sm, offset, &c);

    pio_sm_set_enabled(pio, sm, true);
    pio->txf[sm] = (uint32_t)(half_period_cycles - PIO_CLK_EXTRA_CYCLES);
    return true;
}
