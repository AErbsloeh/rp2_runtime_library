#ifndef HAL_PWM_H_
#define HAL_PWM_H_


#include <stdio.h>
#include "pico/stdlib.h"


/**
 * @file hal_pwm.h
 * @brief PWM Hardware Abstraction Layer for the Raspberry Pi Pico.
 *
 * This HAL abstracts PWM configuration, duty-cycle handling,
 * period changes and interrupt control, following the style of
 * the user's existing Timer HAL.
 */

/* ============================================================
 *                  DATA STRUCTURES
 * ============================================================ */

/**
 * @struct pwm_t
 * @brief Holds all configuration parameters and status values for a PWM module.
 *
 * \param gpio            GPIO pin used for PWM output
 * \param slice           PWM slice number (set automatically from gpio)
 * \param irq_number       IRQ number (typically PWM_IRQ_WRAP)
 * \param use_irq         true = interrupts are used, false = no IRQ
 * \param irq_callback    Function pointer called from the ISR when use_irq is true
 * \param clk_div         Clock divider for the PWM slice (8.4 fixed-point, 1.0 .. 255.9375)
 * \param wrap            PWM wrap value -> determines the frequency
 * \param level           PWM duty cycle (0..wrap)
 * \param enable_state    Current state: PWM active or disabled
 * \param init_done       Flag indicating whether PWM has been initialized
 * \param wrap_irq_flag   Flag used for IRQ synchronization (set on wrap interrupt)
 */
typedef struct {
    uint8_t  gpio;
    uint8_t  slice;
    uint8_t  irq_number;
    bool     use_irq;
    void   (*irq_callback)(void);   /* optional user callback invoked on wrap IRQ */
    float    clk_div;
    uint16_t wrap;
    uint16_t level;
    bool     enable_state;
    bool     init_done;
    bool     wrap_irq_flag;
} pwm_t;

/* ============================================================
 *                  FUNCTION PROTOTYPES
 * ============================================================ */

/**
 * @brief Initializes a PWM instance according to the given handler configuration.
 *
 * Sets the GPIO function, determines the slice, applies clk_div and wrap.
 * Optionally sets up an IRQ (if handler->use_irq == true).
 *
 * @param handler Pointer to a pwm_t structure
 * @return true on successful initialization
 */
bool pwm_hal_init(pwm_t *handler);

/**
 * @brief Enables the PWM output.
 *
 * Applies the duty cycle and enables the corresponding PWM slice.
 *
 * @param handler Pointer to pwm_t
 * @return true if PWM was successfully enabled
 */
bool pwm_hal_enable(pwm_t *handler);

/**
 * @brief Disables the PWM output.
 *
 * @param handler Pointer to pwm_t
 * @return true if PWM was successfully disabled
 */
bool pwm_hal_disable(pwm_t *handler);

/**
 * @brief Sets the PWM level (duty cycle) directly as a 16-bit value.
 *
 * Important: level must be <= wrap.
 *
 * @param handler Pointer to pwm_t
 * @param level   PWM level (0..wrap)
 * @return true on success
 */
bool pwm_hal_set_level(pwm_t *handler, uint16_t level);

/**
 * @brief Updates the duty cycle directly as a 16-bit value.
 *
 * @param handler    Pointer to pwm_t
 * @param duty_cycle Duty cycle (0..wrap)
 * @return true on success
 */
bool pwm_hal_update_duty_cycle(pwm_t *handler, uint16_t duty_cycle);


#endif