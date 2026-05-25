#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hal/clk/pio_clock.h"


#define CLK_PIO_PIN 2
#define CLK_FRQ_VAL	1000


int main(){   
    // Init Phase
    clk_generation_pio_init(pio0, CLK_PIO_PIN, CLK_FRQ_VAL);
	
	// Infinity Loop
    while (true){
		  sleep_ms(1000);
    }
}
