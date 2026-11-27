#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hal/fifo/fifo.h"


int main() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);
    adc_fifo_setup(true, true, 1, false, false);
    adc_set_clkdiv(48000000.0f / 10000.0f);

    fifo_t my_fifo = {
        .element_size = sizeof(uint16_t),
        .length = 256,
        .use_dma = true,
        .dma_source = &adc_hw->fifo,
        .dma_dreq = DREQ_ADC
    };

    if (!fifo_init(&my_fifo)) {
        printf("FIFO init failed\n");
        return 1;
    }
    fifo_dma_start(&my_fifo);
    adc_run(true);

    uint16_t value;
    uint32_t count = 0;
    while (true) {
        if (fifo_pop(&my_fifo, &value)) {
            if (++count % 1000 == 0) {
                printf("ADC: %u (level %u)\n", value, (unsigned)fifo_get_level(&my_fifo));
            }
        }
        tight_loop_contents();
    }
}
