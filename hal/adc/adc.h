#ifndef HAL_ADC_H_
#define HAL_ADC_H_


#include "pico/stdlib.h"


#define RP2_ADC_CH0     0x00
#define RP2_ADC_CH1     0x01
#define RP2_ADC_CH2     0x02
#define RP2_ADC_CH3     0x03
#define RP2_ADC_TEMP    0x04

//TODO: Differenciate between low sampling rate (< 1kHz, polling) and high frequency (else, with FIFO/DMA)

/*! \brief RP2 ADC structure
    \param adc_channel      ADC Channel to be used
    \param sampling_rate    uint32_t value with sampling rate
    \param use_dma          Boolean for using DMA controller in streaming application 
    \param buffersize       size_t with buffer size of the used ADC data buffer
    \param init_done        Initialization done flag
*/
typedef struct {
    uint8_t     adc_channel;
    uint32_t    sampling_rate;
    size_t      buffersize;
    bool        use_dma;
    bool        init_done;  
} rp2_adc_t;


/*! \brief Initialize the internal RP2 ADC peripheral
    \param config       Pointer to the RP2 ADC configuration structure
    \return             true if initialization was successful, false otherwise
*/
bool rp2_adc_init(rp2_adc_t* config);


/*! \brief Change the ADC channel
    \param config        Pointer to the RP2 ADC configuration structure
    \param new_channel   New ADC channel to be used
    \return              true if channel change was successful, false otherwise
*/
bool rp2_adc_change_channel(rp2_adc_t* config, uint8_t new_channel);


/*! \brief Read the raw ADC value as sample from the specified channel
    \param config       Pointer to the RP2 ADC configuration structure
    \return             The raw ADC value as uint16_t but 12-bit are used
*/
uint16_t rp2_adc_read_raw(rp2_adc_t* config);


/*! \brief Function for starting the process to buffer the ADC directly into buffer using Direct Memory Access (DMA)
    \param config       Pointer to the RP2 ADC configuration structure
    \return             Boolean if streaming process is started successful
*/
bool rp2_adc_start_buffer(rp2_adc_t* config);


/*! \brief Function for stopping the process to buffer the ADC directly into buffer using Direct Memory Access (DMA)
    \param config       Pointer to the RP2 ADC configuration structure
    \return             Boolean if streaming process is stopped successful
*/
bool rp2_adc_stop_buffer(rp2_adc_t* config);


/*! \brief Reading the raw ADC data as buffer (polling) from the specified channel
    \param config       Pointer to the RP2 ADC configuration structure
    \return             Boolean if buffer is valid for next processing
*/
bool rp2_adc_read_buffer_polling(rp2_adc_t* config);


/*! \brief Getting the new buffer content
    \return             uint16_t array with ADC data
*/
volatile uint16_t* rp2_adc_get_buffer(rp2_adc_t* config);


/*! \brief Getting the size of each ADC data buffer
    \return size_t with buffersize
*/
size_t rp2_adc_get_buffersize(void);


#endif
