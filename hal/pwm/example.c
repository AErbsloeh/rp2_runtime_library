#include "pico/stdlib.h"
#include "hal/pwm/pwm.h"


#define LED_GPIO 25


int main(void) {
    stdio_init_all();

    pwm_t led = {
        .gpio = LED_GPIO,
        .use_irq = false,
        .clk_div = 255.0f,
        .wrap = 4095,
        .level = 100,
    };

    pwm_hal_init(&led);
    pwm_hal_enable(&led);

    while (true) {
        sleep_ms(1000);
    }

    return 0;
}
