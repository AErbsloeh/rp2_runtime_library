#include "hal/clk/pio_clock.h"
#include "pio_clock.pio.h"
#include "hardware/clocks.h"


bool clk_generation_pio_init(PIO pio, uint pin, uint freq_hz){
    uint sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &blink_program);

    // --- Init Phase
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_pull_down(pin);
    gpio_put(pin, false);

    // --- Defining PIO module
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    pio_sm_config c = blink_program_get_default_config(offset);
    sm_config_set_set_pins(&c, pin, 1);
    pio_sm_init(pio, sm, offset, &c);

     // --- CLK value definition
    pio_sm_set_enabled(pio, sm, true);
    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (clock_get_hz(clk_sys) / (2 * freq_hz)) - 3;
    return true;
}
