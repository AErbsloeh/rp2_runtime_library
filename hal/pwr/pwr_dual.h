#ifndef PWR_WATCH_H_
#define PWR_WATCH_H_


#include "pico/stdlib.h"


// ============================= DECLARATIONS =============================
/*! \brief Struct handler for configuring the power watch module for dual power stage
*   \param pin_en_reg   uint with GPIO number for enabling the power modules of the DCDC regulator
*   \param pin_en_ldo   uint with GPIO number for enabling the power modules of the low dropout regulator
*   \param use_pgd      Boolean for enabling power good feedback and monitor
*   \param use_pgd_isr  Boolean for enabling the power good feedback interrupt service routine (ISR)
*   \param pin_pgd      If true, uint with GPIO number for getting power good feedback
*   \param state        Boolean if power is active and running
*   \param init_done    Boolean for if power watch is initialized
*/
typedef struct {
    uint8_t pin_en_reg;
    uint8_t pin_en_ldo;
    bool use_pgd;
    bool use_pgd_isr;
    uint8_t pin_pgd;
    bool state;
    bool init_done;
} power_dual_t;


// ============================= HELP FUNCTIONS =============================
/*! \brief Function for monitoring the PowerGood GPIO for handling on device
 * \param pin_pgd    Pin number for monitoring the power down signal
 * \param led_state  State of the LED (true: ON, false: OFF)
 * \param config     Pointer to the power configuration struct for dual power supply 
 * \return           Boolean if power down is detected
*/
void gpio_isr_pwr_dual_monitor(uint gpio, uint32_t events, power_dual_t *config);


// ============================= FUNCTIONS FOR DUAL POWER SUPPLY =============================
/*! \brief Function for initializing the dual power supply
 * \param config    Pointer to the power configuration struct for single power supply 
 * \return          Boolean if initialization is done completely
*/
bool init_system_power_dual(power_dual_t *config);

/*! \brief Function for enabling the dual power supply
 * \param config    Pointer to the power configuration struct for single power supply 
 * \return          Boolean if initialization is done completely
*/
bool enable_system_power_dual(power_dual_t *config);

/*! \brief Function for disabling the dual power supply
 * \param config    Pointer to the power configuration struct for single power supply 
 * \return          Boolean if initialization is done completely
*/
bool disable_system_power_dual(power_dual_t *config);


#endif
