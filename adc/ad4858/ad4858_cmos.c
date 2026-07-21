#include "adc/ad4858/ad4858.h"
#include "adc/ad4858/ad4858_cmos.h"
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"


#define NUM_CHANNELS 8
#define NUM_PACKETS_PER_FRAME 9


static ad4858_cmos_t *s_active_cmos = NULL;


static inline void reset_frame_state(ad4858_cmos_t *config)
{
    config->bit_index = 0;
    memset((void *)config->lane_accum, 0, sizeof(config->lane_accum));
}


static void pwm_sck_irq_handler(void)
{
    ad4858_cmos_t *config = s_active_cmos;
    if (!config) {
        uint32_t pending = pwm_get_irq_status_mask();
        for (uint8_t s = 0; s < 12; s++) {
            if (pending & (1u << s)) pwm_clear_irq(s);
        }
        return;
    }

    pwm_clear_irq(config->pwm_slice_sck);

    uint8_t bits[NUM_CHANNELS];
    for (uint8_t lane = 0; lane < NUM_CHANNELS; lane++) {
        bits[lane] = gpio_get(config->gpio_sdo[lane]) ? 1u : 0u;
    }

    for (uint8_t lane = 0; lane < NUM_CHANNELS; lane++) {
        config->lane_accum[lane] = (config->lane_accum[lane] << 1) | bits[lane];
    }

    config->bit_index++;

    unsigned pkt = config->bit_index / config->packet_size;
    unsigned bit_in_pkt = config->bit_index % config->packet_size;

    if (bit_in_pkt == 0) {
        unsigned finished_pkt = pkt - 1;
        if (finished_pkt < NUM_CHANNELS) {
            config->channel_raw[finished_pkt] = config->lane_accum[finished_pkt];
        } else {
            config->status_raw = (uint8_t)config->lane_accum[0];
        }
        memset((void *)config->lane_accum, 0, sizeof(config->lane_accum));
    }

    if (config->bit_index >= (uint16_t)(config->packet_size * NUM_PACKETS_PER_FRAME)) {
        pwm_set_enabled(config->pwm_slice_sck, false);
        config->bit_index = 0;
        config->frame_ready = true;
    }
}

static void gpio_busy_irq_handler(uint gpio, uint32_t events)
{
    ad4858_cmos_t *config = s_active_cmos;
    if (!config || gpio != config->gpio_busy || !(events & GPIO_IRQ_EDGE_FALL)) {
        return;
    }

    reset_frame_state(config);

    pwm_set_counter(config->pwm_slice_sck, 0);
    pwm_set_enabled(config->pwm_slice_sck, true);
}


bool ad4858_init_cmos(ad4858_t* device, ad4858_cmos_t* config){
    gpio_init(config->gpio_convert);
    gpio_set_dir(config->gpio_convert, GPIO_OUT);
    gpio_put(config->gpio_convert, false);

    gpio_init(config->gpio_busy);
    gpio_set_dir(config->gpio_busy, GPIO_IN);

    for(uint8_t i = 0; i < NUM_CHANNELS; i++){
        gpio_init(config->gpio_sdo[i]);
        gpio_set_dir(config->gpio_sdo[i], GPIO_IN);
    }

    gpio_init(config->gpio_cmos_sck);
    gpio_set_dir(config->gpio_cmos_sck, GPIO_OUT);
    gpio_put(config->gpio_cmos_sck, false);

    switch(ad4858_get_packet_size(device)){
        case AD4858_PACKETSIZE_20BIT:
            config->packet_size = 20;
            break;
        case AD4858_PACKETSIZE_24BIT:
            config->packet_size = 24;
            break;
        case AD4858_PACKETSIZE_32BIT:
            config->packet_size = 32;
            break;
    }

    config->pwm_slice_cnv = pwm_gpio_to_slice_num(config->gpio_convert);
    config->pwm_slice_sck = pwm_gpio_to_slice_num(config->gpio_cmos_sck);
    if (config->pwm_slice_cnv == config->pwm_slice_sck) {
        return false;
    }

    reset_frame_state(config);
    memset((void *)config->channel_raw, 0, sizeof(config->channel_raw));
    config->status_raw = 0;
    config->frame_ready = false;
    config->running = false;

    s_active_cmos = config;

    return true;
}


bool ad4858_do_single_conversion(ad4858_cmos_t* config){
    gpio_put(config->gpio_convert, true);
    sleep_us(1);
    gpio_put(config->gpio_convert, false);
    return true;
}


bool ad4858_start_continuous_conversion(ad4858_cmos_t* config){
    if (config->running) {
        return false;
    }
    if (config->cmos_sclk_frequency == 0 || config->sampling_period_us == 0) {
        return false;
    }

    uint32_t sys_clk_hz = clock_get_hz(clk_sys);

    gpio_set_function(config->gpio_convert, GPIO_FUNC_PWM);
    {
        uint32_t period_ticks = (uint32_t)(((uint64_t)sys_clk_hz * config->sampling_period_us) / 1000000u);
        if (period_ticks < 4) period_ticks = 4;

        uint32_t high_ticks = period_ticks / 20;
        if (high_ticks < 1) high_ticks = 1;

        pwm_config c = pwm_get_default_config();
        pwm_config_set_wrap(&c, (uint16_t)(period_ticks - 1u));
        pwm_init(config->pwm_slice_cnv, &c, false);
        pwm_set_gpio_level(config->gpio_convert, (uint16_t)high_ticks);
        pwm_set_irq_enabled(config->pwm_slice_cnv, false);
    }

    gpio_set_function(config->gpio_cmos_sck, GPIO_FUNC_PWM);
    {
        uint32_t period_ticks = sys_clk_hz / config->cmos_sclk_frequency;
        if (period_ticks < 4) period_ticks = 4;

        pwm_config c = pwm_get_default_config();
        pwm_config_set_wrap(&c, (uint16_t)(period_ticks - 1u));
        pwm_init(config->pwm_slice_sck, &c, false);
        pwm_set_gpio_level(config->gpio_cmos_sck, (uint16_t)(period_ticks / 2));
        pwm_set_irq_enabled(config->pwm_slice_sck, true);
    }

    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_sck_irq_handler);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    gpio_set_irq_enabled_with_callback(config->gpio_busy, GPIO_IRQ_EDGE_FALL, true, &gpio_busy_irq_handler);

    reset_frame_state(config);
    config->frame_ready = false;
    config->running = true;

    pwm_set_enabled(config->pwm_slice_cnv, true);

    return true;
}


bool ad4858_stop_continuous_conversion(ad4858_cmos_t* config){
    if (!config->running) {
        return false;
    }

    gpio_set_irq_enabled(config->gpio_busy, GPIO_IRQ_EDGE_FALL, false);
    pwm_set_irq_enabled(config->pwm_slice_sck, false);

    pwm_set_enabled(config->pwm_slice_cnv, false);
    pwm_set_enabled(config->pwm_slice_sck, false);

    gpio_set_function(config->gpio_convert, GPIO_FUNC_SIO);
    gpio_set_dir(config->gpio_convert, GPIO_OUT);
    gpio_put(config->gpio_convert, false);

    gpio_set_function(config->gpio_cmos_sck, GPIO_FUNC_SIO);
    gpio_set_dir(config->gpio_cmos_sck, GPIO_OUT);
    gpio_put(config->gpio_cmos_sck, false);

    config->running = false;
    return true;
}


bool ad4858_cmos_frame_ready(ad4858_cmos_t* config){
    return config->frame_ready;
}


void ad4858_cmos_get_frame(ad4858_cmos_t* config, uint32_t out_channels[8], uint8_t* out_status){
    pwm_set_irq_enabled(config->pwm_slice_sck, false);

    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        out_channels[i] = config->channel_raw[i];
    }
    if (out_status) {
        *out_status = config->status_raw;
    }
    config->frame_ready = false;

    pwm_set_irq_enabled(config->pwm_slice_sck, true);
}


int32_t ad4858_cmos_extract_result20(uint32_t packet_raw, uint8_t packet_size_bits){
    int32_t raw20 = (int32_t)((packet_raw >> (packet_size_bits - 20u)) & 0xFFFFFu);
    if (raw20 & 0x80000) {
        raw20 -= (1 << 20);
    }
    return raw20;
}