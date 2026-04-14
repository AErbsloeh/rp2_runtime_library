#include "sens/max9814/max9814.h"


bool mic_amp_max9814_init(mic_t* config){
    gpio_init(config->gpio_enable);
    gpio_set_dir(config->gpio_enable, GPIO_OUT);
    gpio_put(config->gpio_enable, false);

    mic_amp_max9814_set_gain(config, config->mode_gain);
    mic_amp_max9814_set_ar(config, config->mode_ar);

    config->init_done = true;
    return config->init_done;
}


bool mic_amp_max9814_enable(mic_t* config, bool state){
    gpio_put(config->gpio_enable, state);
    return true;
}


bool mic_amp_max9814_set_gain(mic_t* config, uint8_t mode){
    config->mode_gain = mode;

    switch(mode){
        case MIC_MAX9814_GAIN_50DB:
            gpio_init(config->gpio_gain);
            gpio_set_dir(config->gpio_gain, GPIO_OUT);
            gpio_put(config->gpio_gain, false);
        break;
        case MIC_MAX9814_GAIN_60DB:
            gpio_init(config->gpio_gain);
            gpio_set_dir(config->gpio_gain, GPIO_IN);
        break;
        default: // --> 40 dB
            gpio_init(config->gpio_gain);
            gpio_set_dir(config->gpio_gain, GPIO_OUT);
            gpio_put(config->gpio_gain, true);
        break;
    }
    return true;
}


bool mic_amp_max9814_set_ar(mic_t* config, uint8_t mode){
    config->mode_ar = mode;

    switch(mode){
        case MIC_MAX9814_AR_1_2000:
            gpio_init(config->gpio_ar);
            gpio_set_dir(config->gpio_ar, GPIO_OUT);
            gpio_put(config->gpio_ar, true);
        break;
        case MIC_MAX9814_AR_1_4000:
            gpio_init(config->gpio_ar);
            gpio_set_dir(config->gpio_ar, GPIO_IN);
        break;
        default: // --> ratio = 1:500
            gpio_init(config->gpio_ar);
            gpio_set_dir(config->gpio_ar, GPIO_OUT);
            gpio_put(config->gpio_ar, false);
        break;
    }
    return true;
}