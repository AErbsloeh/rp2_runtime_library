#ifndef PWR_WATCH_SINGLE_H_
#define PWR_WATCH_SINGLE_H_


#include "pico/stdlib.h"


// ============================= DECLARATIONS =============================
/*! \brief Struct handler for configuring the power watch module for single power stage
*   \param pin_en       uint with GPIO number for enabling the power modules
*   \param use_pgd      Boolean for enabling power good feedback and monitor
*   \param pin_pgd      If true, uint with GPIO number for getting power good feedback
*   \param state        Boolean if power is active and running
*   \param init_done    Boolean for if power watch is initialized
*/
typedef struct {
    uint8_t pin_en;
    bool use_pgd;
    uint8_t pin_pgd;
    bool state;
    bool init_done;
} power_single_t;


// ============================= HELP FUNCTIONS =============================
/*! \brief Function for monitoring the PowerGood GPIO for handling on device
 * \param pin_pgd    Pin number for monitoring the power down signal
 * \param led_state  State of the LED (true: ON, false: OFF)
 * \param config     Pointer to the power configuration struct for single power supply 
 * \return           Boolean if power down is detected
*/
void gpio_isr_pwr_single_monitor(uint gpio, uint32_t events, power_single_t *config);


// ============================= FUNCTIONS FOR SINGLE POWER SUPPLY =============================
/*! \brief Function for initializing the single power supply
 * \param config    Pointer to the power configuration struct for single power supply 
 * \return          Boolean if initialization is done completely
*/
bool init_system_power_single(power_single_t *config);

/*! \brief Function for enabling the single power supply
 * \param config    Pointer to the power configuration struct for single power supply 
 * \return          Boolean if initialization is done completely
*/
bool enable_system_power_single(power_single_t *config);

/*! \brief Function for disabling the single power supply
 * \param config    Pointer to the power configuration struct for single power supply 
 * \return          Boolean if initialization is done completely
*/
bool disable_system_power_single(power_single_t *config);


#endif
