#include "hal/usb/usb.h"
#include <stdlib.h>
#include <stdio.h>


// ============================== FUNCTIONS FOR PROCESSING ==============================
bool usb_init(usb_rp2_t* config){
    stdio_init_all();
	stdio_set_translate_crlf(&stdio_usb, false);

    config->data = malloc(config->length * sizeof(char));
    for(size_t idx=0; idx < config->length; idx++){
        config->data[idx] = 0;
    };
    return true;
}


bool usb_wait_until_connected(void){
    while (!stdio_usb_connected()){
        sleep_ms(500);
    };
    return true;
}


void usb_handling_fifo_buffer(usb_rp2_t* config){
    int charac = getchar_timeout_us(0);
    if (charac == PICO_ERROR_TIMEOUT){
        config->ready = false;
        return;
    }

    config->data[config->position] = (char)charac;
    if(config->position == 0) {
        config->position = config->length -1;
        config->ready = true;
    } else {
        config->position--;
        config->ready = false;
    }
};


size_t usb_send_bytes(char* buffer, size_t num_bytes){
    size_t written = fwrite(buffer, 1, num_bytes, stdout);
    fflush(stdout);
    return written;
}


uint16_t calc_checksum(char* data, size_t len){
    uint16_t sum = 0;    
    for (size_t idx=0; idx < len; idx++){
        sum += data[idx];
    };
    return sum;
}
