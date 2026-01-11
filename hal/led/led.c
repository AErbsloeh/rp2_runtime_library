#include "hal/led/led.h"
#include "hardware/gpio.h"
#ifdef LED_CYW43_SUPPORTED
    #include "pico/cyw43_arch.h"
#elif LED_WS2812_SUPPORTED
	#include "peri/ws2812/ws2812.h"
#endif

// ======================================== INTERNAL FUNCTIONS ===============================================
uint8_t pin_used = 25;	// Default pin of board LED (Pico / Pico 2)
bool led_state = false;


// ======================================== CALLABLE FUNCTIONS ===============================================
void set_gpio_default_led(uint8_t led_pin){
	#ifdef LED_CYW43_SUPPORTED
		pin_used = CYW43_WL_GPIO_LED_PIN;
	#elif LED_WS2812_SUPPORTED
		pin_used = 17;
	#else
		pin_used = led_pin;
	#endif
};


uint8_t get_gpio_default_led(void){
	return pin_used;
};


void init_default_led(void){
	led_state = false;
	#ifdef LED_CYW43_SUPPORTED
		if (cyw43_arch_init()) {
            return false;
        }
		set_gpio_default_led(255);
		cyw43_arch_gpio_put(pin_used, led_state);
	#elif LED_WS2812_SUPPORTED
		set_gpio_default_led(255);
		PIO  pio = pio0;
		ws2812_init(pio, pin_used);
		put_pixel_rgb(0, 0, 0); 		
	#else
		gpio_init(pin_used);
		gpio_set_dir(pin_used, GPIO_OUT);
		gpio_put(pin_used, led_state);
	#endif
};


bool set_state_default_led(bool state){
	led_state = state;
	#ifdef LED_CYW43_SUPPORTED 
		cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state);
	#elif LED_WS2812_SUPPORTED
		if(state) 
			put_pixel_rgb(0, 32, 0);
		else
			put_pixel_rgb(0, 0, 0);
	#else
		gpio_put(pin_used, state);
	#endif
	return led_state;
};


bool get_state_default_led(void){
	#ifdef LED_CYW43_SUPPORTED
		led_state = cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN);
	#elif LED_WS2812_SUPPORTED
		led_state = led_state;
	#else
		led_state = gpio_get(pin_used);
	#endif
	return led_state;
};


bool toggle_state_default_led(void){
	set_state_default_led(!get_state_default_led());
	return get_state_default_led();
};
