#include "hal/pwm/pwm.h"
#include "hardware/pwm.h"


#define PWM_HAL_MAX_SLICES 12


static pwm_t *pwm_hal_slice_owner[PWM_HAL_MAX_SLICES] = {0};
static bool pwm_hal_irq_installed[32] = {0};


static void pwm_hal_shared_irq_handler(void) {
    uint32_t status = pwm_get_irq_status_mask();

    for (uint slice = 0; slice < PWM_HAL_MAX_SLICES; slice++) {
        if (status & (1u << slice)) {
            pwm_clear_irq(slice);

            pwm_t *owner = pwm_hal_slice_owner[slice];
            if (owner != NULL) {
                owner->wrap_irq_flag = true;
                if (owner->irq_callback != NULL) {
                    owner->irq_callback();
                }
            }
        }
    }
}

bool pwm_hal_init(pwm_t *handler) {
    if (handler == NULL) {
        return false;
    }

    if (handler->init_done) {
        return true;
    }

    gpio_set_function(handler->gpio, GPIO_FUNC_PWM);
    handler->slice = pwm_gpio_to_slice_num(handler->gpio);

    pwm_set_clkdiv(handler->slice, handler->clk_div);
    pwm_set_wrap(handler->slice, handler->wrap);

    if (handler->use_irq) {
        if (handler->slice >= PWM_HAL_MAX_SLICES) {
            return false;
        }

        pwm_hal_slice_owner[handler->slice] = handler;

        pwm_clear_irq(handler->slice);
        pwm_set_irq_enabled(handler->slice, true);

        if (!pwm_hal_irq_installed[handler->irq_number]) {
            irq_set_exclusive_handler(handler->irq_number, pwm_hal_shared_irq_handler);
            irq_set_enabled(handler->irq_number, true);
            pwm_hal_irq_installed[handler->irq_number] = true;
        }

        handler->wrap_irq_flag = false;
    }

    handler->init_done = true;
    return true;
}

bool pwm_hal_enable(pwm_t *handler) {
    if (handler == NULL) {
        return false;
    }

    if (!handler->init_done) {
        if (!pwm_hal_init(handler)) {
            return false;
        }
    }

    pwm_set_gpio_level(handler->gpio, handler->level);
    pwm_set_enabled(handler->slice, true);

    handler->enable_state = true;
    return true;
}

bool pwm_hal_disable(pwm_t *handler) {
    if (handler == NULL) {
        return false;
    }

    if (!handler->init_done) {
        if (!pwm_hal_init(handler)) {
            return false;
        }
    }

    pwm_set_enabled(handler->slice, false);

    handler->enable_state = false;
    return true;
}

bool pwm_hal_set_level(pwm_t *handler, uint16_t level) {
    if (handler == NULL) {
        return false;
    }

    if (!handler->init_done) {
        if (!pwm_hal_init(handler)) {
            return false;
        }
    }

    if (level > handler->wrap) {
        return false;
    }

    handler->level = level;
    pwm_set_gpio_level(handler->gpio, level);
    return true;
}

bool pwm_hal_update_duty_cycle(pwm_t *handler, uint16_t duty_cycle) {
    return pwm_hal_set_level(handler, duty_cycle);
}
