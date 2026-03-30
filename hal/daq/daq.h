#ifndef DAQ_SAMPLE_H_
#define DAQ_SAMPLE_H_


#include "hal/tmr/tmr.h"
#include "hal/daq/fifo.h"


/*! \brief Data structure for DAQ sampling data packet
    \param num_channels Number of channels sampled
    \param num_samples  Number of samples taken
    \param packet_id    Identifier for the data packet type
    \param iteration    Iteration count of the sampling
    \param runtime      Actual runtime in microseconds since system start
    \param send_batch   Flag to indicate if data should be sent batch-wise (true) or sample-wise (false)
    \param new_data     Flag to indicate if new data is available in the FIFO
    \param data         Sampled data value from channels
*/
typedef struct {
    uint16_t const num_channels;
    uint16_t const num_samples;
    uint8_t const packet_id;
    uint8_t volatile iteration;
    uint64_t volatile runtime;
    bool volatile send_batch;
    bool volatile new_data;
    fifo_t* data;
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


/*! \brief Function to send the DAQ data packet via USB
* \param data           Pointer to the DAQ data packet
* \param frame_size     Size of the USB frame in bytes
* \param num_samples    Number of samples to send
*/
void daq_send_data_usb(daq_data_t* data, size_t num_samples);


#endif
