#ifndef HAL_TMR_H_
#define HAL_TMR_H_


#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"


// ========================================= DEFINITIONS ============================================
/*! \brief Struct definition for handling an ISR as timer
 * \param timer              Repeating timer handler (used by enable_repeat_timer_irq() and disable_repeat_timer_irq())
 * \param irq_number         32-bit integer as timer IRQ number (not used by the functions anymore, only kept for existing initialisers)
 * \param period_us          64-bit integer as timer period in microseconds (<0 repeat to every period, >0 wait period before start; one-shot timer: absolute value is the delay)
 * \param alarm_done         Boolean, true after the timer was initialised with init_timer_irq()
 * \param enable_state       Boolean for the timer enable state
 * \param init_done          Boolean for the timer initialization state
 * \param func_irq           Function pointer for the timer ISR of the repeating timer (return true to keep the timer running)
 * \param func_alarm         Function pointer for the ISR of the one-shot timer (optional, only used by activate_oneshot_timer_irq())
 * \param alarm_id           ID of the one-shot timer (0 if none is pending)
*/
typedef struct {
    repeating_timer_t *timer;
    uint32_t irq_number;
    int64_t period_us;
    bool alarm_done;
    bool enable_state;
    bool init_done;
    bool (*func_irq)(repeating_timer_t *rt);
    alarm_callback_t func_alarm;
    alarm_id_t alarm_id;
} tmr_repeat_irq_t;


// ======================================== FUNCTIONS ===============================================
/*! \brief Function for initialization of a repeating timer using an interrupt service routine (ISR) on Pico
* The timers of the SDK (alarm pool) need no further setup, so the function only marks the handler as initialised
* \param handler      Struct handler for the repeating timer
* \return             Boolean for done initialisation of timer ISR
*/
bool init_timer_irq(tmr_repeat_irq_t* handler);


/*! \brief Enabling an repeating timer using an interrupt service routine (ISR) on Pico
* \param handler      Struct handler for the repeating timer (needs timer and func_irq)
* \return             Boolean for the enabled timer (false if the timer could not be created)
*/
bool enable_repeat_timer_irq(tmr_repeat_irq_t* handler);


/*! \brief Disabling an repeating timer (and a pending one-shot timer) using an interrupt service routine (ISR) on Pico
* \param handler      Struct handler for the repeating timer
* \return             Boolean for the deactivated timer (also true if the timer was not running)
*/
bool disable_repeat_timer_irq(tmr_repeat_irq_t* handler);


/*! \brief Activating a one-shot timer, which calls func_alarm once after the delay in period_us
* \param handler      Struct handler for the timer (needs func_alarm, gets the pointer as user data of the callback)
* \return             Boolean for the armed one-shot timer (false if the alarm could not be created)
*/
bool activate_oneshot_timer_irq(tmr_repeat_irq_t* handler);


#endif
