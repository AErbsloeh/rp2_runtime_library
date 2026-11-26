#ifndef PWR_WATCH_SINGLE_H_
#define PWR_WATCH_SINGLE_H_


#include "pico/stdlib.h"


// ============================= DECLARATIONS =============================
/*! \brief Struct handler for configuring the power watch module for single power stage
*   \param pin_en       uint with GPIO number for enabling the power modules
*   \param use_pgd      Boolean for enabling power good feedback and monitor
*   \param use_pgd_isr  Boolean for enabling the power good feedback interrupt service routine (ISR)
*   \param pin_pgd      If true, uint with GPIO number for getting power good feedback (active high: high = power is good, low = not good, the pin gets a pull-down)
*   \param state        Boolean if power is active and running
*   \param init_done    Boolean for if power watch is initialized
*/
typedef struct {
    uint8_t pin_en;
    bool use_pgd;
    bool use_pgd_isr;
    uint8_t pin_pgd;
    bool state;
    bool init_done;
} power_single_t;


// ============================= HELP FUNCTIONS =============================
/*! \brief Function for the handling of a lost power good signal, call it from the GPIO callback of pin_pgd (falling edge)
 * It switches the supply off and disables the interrupt without any delay, so it can be used in an interrupt service routine
 * \param gpio       GPIO number of the interrupt
 * \param events     Events of the interrupt
 * \param config     Pointer to the power configuration struct for single power supply
*/
void gpio_isr_pwr_single_monitor(uint gpio, uint32_t events, power_single_t *config);


/*! \brief Function for checking that the power good signal stays high for 100 ms
 * \param pin_pgd    Pin number for monitoring the power good signal
 * \return          Boolean if the power good signal was high the whole time
*/
bool monitor_system_power_single_pgd_start(uint8_t pin_pgd);


// ============================= FUNCTIONS FOR SINGLE POWER SUPPLY =============================
/*! \brief Function for initializing the single power supply
 * \param config    Pointer to the power configuration struct for single power supply 
 * \return          Boolean if initialization is done completely
*/
bool init_system_power_single(power_single_t *config);

/*! \brief Function for enabling the single power supply
 * With power good feedback the supply is enabled and the power good signal has to get high within 4 s and stay high for 100 ms, otherwise the supply is switched off again.
 * Without feedback the supply is enabled after 400 ms and the function waits another 400 ms. With use_pgd_isr a falling edge of the power good signal is detected afterwards (the GPIO callback has to call gpio_isr_pwr_single_monitor())
 * \param config    Pointer to the power configuration struct for single power supply 
 * \return          Boolean if initialization is done completely
*/
bool enable_system_power_single(power_single_t *config);

/*! \brief Function for disabling the single power supply (waits 100 ms, so do not call it from an interrupt service routine)
 * \param config    Pointer to the power configuration struct for single power supply 
 * \return          Boolean if initialization is done completely
*/
bool disable_system_power_single(power_single_t *config);


#endif
