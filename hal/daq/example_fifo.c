#include "pico/stdlib.h"
#include "hal/daq/fifo.h"


int main() {
    stdio_init_all();
// creating a fifo buffer named my_fifo with 1 channel, 16 samples, and each sample is 1 byte (uint8_t)
   /* fifo_t my_fifo1 = {
        //.channel_num = 1,
        .element_size = sizeof(uint8_t),
        .length = 16
    };

    fifo_init(&my_fifo1);
    uint8_t value = 0; // the new data that we want to push into the fifo buffer, in this example we will just push an incrementing value
    uint8_t out; // this is where we will store the popped values from the fifo buffer 

    while (true) {
        fifo_push(&my_fifo1, &value);
        if (fifo_is_full(&my_fifo1)) {
            printf("Buffer is full\n");
            for(size_t i = 0; i < my_fifo1.length; i++){
                fifo_pop(&my_fifo1, &out);
                printf("%d, ", out);
            }
            printf("]\n");
        }
        value++;
        tight_loop_contents();
    }
    fifo_t my_fifo2 = {
        //.channel_num = 1,
        .element_size = sizeof(uint8_t),
        .length = 16
    };

    fifo_init(&my_fifo2);
    uint8_t value = 0;
    uint8_t out;

    while (true) {
        fifo_push(&my_fifo2, &value);
        if (fifo_is_full(&my_fifo2)) {
            printf("Buffer is full\n");
            for(size_t i = 0; i < my_fifo2.length; i++){
                fifo_pop(&my_fifo2, &out);
                printf("%d, ", out);
            }
            printf("]\n");
        }
        value++;
        tight_loop_contents();
    }*/

    double_buffer_t my_double_buffer{
    .fifo1 = {
        .element_size = sizeof(uint8_t),
        .length = 16
    },
    .fifo2 = {
        .element_size = sizeof(uint8_t),
        .length = 16
    }
};
double_buffer_init(&my_double_buffer);
uint8_t value = 0;
uint8_t out;

while(true){
    double_buffer_push(&my_double_buffer, &value);
    if(double_buffer_switch(&my_double_buffer)){
        printf("Switched buffers\n");
        for(size_t i = 0; i < my_double_buffer.read_fifo->length; i++){
            double_buffer_pop(&my_double_buffer, &out);
            printf("%d, ", out);
        }
        printf("]\n");
    }
    value++;
    tight_loop_contents();
}
}

