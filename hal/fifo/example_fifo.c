#include <stdio.h>
#include "pico/stdlib.h"
#include "hal/fifo/fifo.h"


int main() {
    stdio_init_all();

    fifo_t my_fifo = {
        .element_size = sizeof(uint8_t),
        .length = 16
    };

    fifo_init(&my_fifo);
    uint8_t value = 0;
    uint8_t out;

    while (true) {
        fifo_push(&my_fifo, &value);
        if (fifo_is_full(&my_fifo)) {
            printf("Buf: [");
            for(size_t i = 0; i < my_fifo.length; i++){
                fifo_pop(&my_fifo, &out);
                printf("%d, ", out);
            }
            printf("]\n");
        }
        value++;
        tight_loop_contents();
    }
}