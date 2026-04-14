#ifndef MIC_MAC9814_H_
#define MIC_MAC9814_H_


#include "pico/stdlib.h"


#define MIC_MAX9814_GAIN_40DB   0
#define MIC_MAX9814_GAIN_50DB   1
#define MIC_MAX9814_GAIN_60DB   2
#define MIC_MAX9814_AR_1_500    0
#define MIC_MAX9814_AR_1_2000   1
#define MIC_MAX9814_AR_1_4000   2


// More informations on: https://www.analog.com/media/en/technical-documentation/data-sheets/max9814.pdf
/*! \brief 
    \param gpio_enable  GPIO pin number for defining the ~{SHUTDOWN} pin for enabling device
    \param gpio_ar      GPIO pin number for defining the trilevel attack and realse time ratio (A/R)
    \param gpio_gain    GPIO pin number for defining the gain pin
    \param mode_ar      uint8_t for defining the A/R ratio (0= 1:500, 1= 1:2000, 2= 1:4000)
    \param mode_gain    uint8_t for defining the gain (0= 40dB, 1= 50dB, 2= 60dB)
    \param init_done    Boolean for if initialization is done
*/
typedef struct{
    uint8_t gpio_enable;
    uint8_t gpio_ar;
    uint8_t gpio_gain;
    uint8_t mode_ar;
    uint8_t mode_gain;
    bool init_done;
}   mic_t;


/*! \brief Function for defining the gain of the microphone amplifier MAC9814 from Analog Devices
    \param config       Pointer to struct for configuring the device
    \param mode         uint8_t value with new gain (0= 40dB, 1= 50dB, 2=60dB)
    \return             Boolean if initialization is done successful
*/  
bool mic_amp_max9814_init(mic_t* config);


/*! \brief Function for defining the enable state of the microphone amplifier MAC9814 from Analog Devices
    \param config       Pointer to struct for configuring the device
    \param state        Boolean with state to enable (=1) or disable (=0) the device
    \return             Boolean if setting is done successful
*/  
bool mic_amp_max9814_enable(mic_t* config, bool state);


/*! \brief Function for defining the gain of the microphone amplifier MAC9814 from Analog Devices
    \param config       Pointer to struct for configuring the device
    \param mode         uint8_t value with new gain (0= 40dB, 1= 50dB, 2=60dB)
    \return             Boolean if setting is done successful
*/  
bool mic_amp_max9814_set_gain(mic_t* config, uint8_t mode);


/*! \brief Function for defining the attack and release ratio (A/R) of the microphone amplifier MAC9814 from Analog Devices
    \param config       Pointer to struct for configuring the device
    \param mode         uint8_t value with new A/R ratio (0= 1:500, 1= 1:2000, 2= 1:4000)
    \return             Boolean if setting is done successful
*/  
bool mic_amp_max9814_set_ar(mic_t* config, uint8_t mode);


#endif
