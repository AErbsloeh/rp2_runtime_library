#ifndef DAQ_SAMPLE_H_
#define DAQ_SAMPLE_H_


#include "hal/tmr/tmr.h"
#include "hal/daq/fifo.h"


#define DAQ_MODE_SAMPLE         0x00
#define DAQ_MODE_BUFFER_SINGLE  0x01
#define DAQ_MODE_BUFFER_DOUBLE  0x02


/*! \brief Data structure for DAQ sampling data packet
    \param num_channels     Number of channels sampled
    \param num_samples      Number of samples taken
    \param packet_id        Identifier for the data packet type
    \param iteration        Iteration count of the sampling
    \param runtime_first    Runtime of the first sample in microseconds since system start
    \param runtime_last     Runtime of the last sample in microseconds since system start
    \param is_signed        Flag to indicate if the sampled data is signed
    \param element_size     Number of bytes of the used data type
    \param send_mode        Flag to indicate the DAQ mode (using defines DAQ_MODE_*)
    \param new_data         Flag to indicate if new data is available in the FIFO
    \param first_buffer_full    Boolean to indicate that first buffer is ready for transmitting
    \param data             Sampled data value from channels
*/
typedef struct {
    uint16_t const num_channels;
    uint16_t const num_samples;
    uint8_t const packet_id;
    uint8_t volatile iteration;
    uint64_t volatile runtime_first;
    uint64_t volatile runtime_last;
    bool const is_signed;
    uint8_t const element_size;
    uint8_t volatile send_mode;
    bool volatile new_data;
    bool volatile first_buffer_full;
    fifo_t* data0;
    fifo_t* data1;
} daq_data_t;


/*! \brief Function to init the DAQ sampling unit
    \param handler    Pointer to the timer IRQ handler structure
    \param data       Pointer to the DAQ data structure
    \return           true if initialization was successful, false otherwise
*/
bool daq_init_sampling(tmr_repeat_irq_t* handler, daq_data_t* data);


/*! \brief Function to update the sampling rate of the DAQ sampling unit
* \param handler        Pointer to the timer IRQ handler structure
* \param new_rate_us    New sampling rate in microseconds
* \return               State of the DAQ sampling unit (true=running, false=deactivated)
*/
bool daq_update_sampling_rate(tmr_repeat_irq_t* handler, int64_t new_rate_us);


/*! \brief Function to start the DAQ sampling unit
* \param handler        Pointer to the timer IRQ handler structure
* \return               State of the DAQ sampling unit (true=running, false=deactivated)
*/
bool daq_start_sampling(tmr_repeat_irq_t* handler);


/*! \brief Function to stop the DAQ sampling unit
* \param handler        Pointer to the timer IRQ handler structure
* \return               State of the DAQ sampling unit (true=deactivated, false=activated)
*/
bool daq_stop_sampling(tmr_repeat_irq_t* handler);


/*! \brief Function to push data to the DAQ FIFO
* \param data           Pointer to the DAQ data structure
* \param new_data_in    Pointer to the new data to be pushed
* \return               true if data was pushed successfully, false otherwise
*/
bool daq_push_data_to_fifo(daq_data_t* data, void* new_data_in);


/*! \brief Function to pop data from the DAQ FIFO
* \param data           Pointer to the DAQ data structure
* \param data_out       Pointer to the buffer where the popped data will be stored
* \return               true if data was popped successfully, false otherwise
*/
bool daq_pop_data_from_fifo(daq_data_t* data, void* data_out);


/*! \brief Function to check if the DAQ FIFO is full
* \param data           Pointer to the DAQ data structure
* \return               true if the FIFO is full, false otherwise
*/
bool daq_is_fifo_full(daq_data_t* data);


/*! \brief Function to check if the DAQ FIFO is empty
* \param data               Pointer to the DAQ data structure
* \return                   true if the FIFO is empty, false otherwise
*/
bool daq_is_empty_fifo(daq_data_t* data);


/*! \brief Function to check if the DAQ data is ready to be sent
* \param data           Pointer to the DAQ data structure
* \return               true if data is ready to be sent, false otherwise
*/
bool daq_check_send_data(daq_data_t* data);


/*! \brief Function to send the DAQ data packet via the selected transport
* \param data           Pointer to the DAQ data packet
*/
void daq_send_data(daq_data_t* data);


/*! \brief Function for processing the data in th DAQ IRQ
*   \param config       Pointer to the DAQ config struct
*   \param data         Pointer to the DAQ data packet
*   \return             Boolean (always true) for running the Timer IRQ queue
*/
bool daq_irq_process(daq_data_t* config, void* data);


/*! \brief Function to calculate the number of bytes per single-sample DAQ frame
    \param data       Pointer to the DAQ data structure
    \return           Number of bytes per single-sample DAQ frame
*/
uint16_t daq_get_number_bytes_per_sample(daq_data_t* data);


/*! \brief Function to calculate the number of bytes per batch DAQ frame
    \param data       Pointer to the DAQ data structure
    \return           Number of bytes per batch DAQ frame
*/
uint16_t daq_get_number_bytes_per_batch(daq_data_t* data);


#endif
