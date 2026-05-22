#ifndef HAL_LED_H_
#define HAL_LED_H_


#include "pico/stdlib.h"


// ============================== INTERNAL FUNCTIONS =========================================
/*! Function for setting the GPIO pin number of the default LED.
 * @param led_pin: The GPIO pin number for the LED (if not cyw43 LED is used).
 * @return None
 */
void set_gpio_default_led(uint8_t led_pin);


/*! Function for getting the GPIO pin number of the default LED.
 * @return The GPIO pin number for the LED (if not cyw43 LED is used).
 */
uint8_t get_gpio_default_led(void);


/*! Function for initializing the default LED.
 * @return Returns true if initialization was successful, false otherwise.
 */
bool init_default_led(void);


/*! Function for setting the state of the default LED.
 * @param state: The state to set the LED to (true for on, false for off).
 * @return       The state of the LED after setting it.
 */
bool set_state_default_led(bool state);


/*! Function for getting the state of the default LED.
 * @return          The actual state of the LED.
 */
bool get_state_default_led(void);


/*! Function for toggling the state of the default LED.
 * @return          The actual state of the LED.
 */
bool toggle_state_default_led(void);


#ifdef USE_WS2812_DEFAULT || LED_KB2040_SUPPORTED
    /*! Function for setting the color of the default LED.
     * @param red: The red component of the color (0-255).
     * @param green: The green component of the color (0-255).
     * @param blue: The blue component of the color (0-255).
     * @return None
     */
    void set_color_default_led(uint8_t red, uint8_t green, uint8_t blue);
#endif
#endif

