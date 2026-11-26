#ifndef DAQ_SAMPLE_H_
#define DAQ_SAMPLE_H_


#include "hal/tmr/tmr.h"
#include "hal/daq/daq_buffer.h"


#define DAQ_MODE_SAMPLE         0x00
#define DAQ_MODE_BUFFER         0x01
#define DAQ_MODE_PINGPONG       0x02

#define DAQ_TYPE_SAMPLE         0x00
#define DAQ_TYPE_CONT_WINDOW    0x01
#define DAQ_TYPE_EVENT_WINDOW   0x02
#define DAQ_TYPE_FEATURE        0x03
#define DAQ_TYPE_EVENT          0x04
#define DAQ_TYPE_CLASS          0x05


/*! \brief Data structure for DAQ sampling data packet
    \param num_channels     Number of channels sampled
    \param num_samples      Number of samples taken
    \param packet_id        Identifier for the data packet type (head)
    \param packet_tail      Identifier for the data packet type (tail)
    \param iteration        Iteration count of the sampling
    \param runtime_first    Runtime of the first sample in microseconds since system start
    \param runtime_last     Runtime of the last sample in microseconds since system start
    \param is_signed        Flag to indicate if the sampled data is signed
    \param element_size     Number of bytes of the used data type
    \param send_mode        Flag to indicate the DAQ mode (using defines DAQ_MODE_*)
    \param data_type        Kind of the data in the stream (using defines DAQ_TYPE_*), default (not set) is DAQ_TYPE_SAMPLE
                            DAQ_TYPE_SAMPLE:        Values of single samples with a constant rate (raw data, after filtering or echo)
                            DAQ_TYPE_CONT_WINDOW:   Windows with a fixed step (one after another or overlapping), continuous and regular in time
                            DAQ_TYPE_EVENT_WINDOW:  Windows around an event (trigger), irregular in time
                            DAQ_TYPE_FEATURE:       Features of a window (e.g. mean or RMS), one value or vector for each window
                            DAQ_TYPE_EVENT:         Irregular events (e.g. threshold or spike detection), the timestamp is the information
                            DAQ_TYPE_CLASS:         Classification result (label) of a window
    \param new_data         Flag to indicate if a complete block waits for being sent
    \param buffer           Pointer to the buffer of the stream, used by all modes (DAQ_MODE_SAMPLE: one sample, DAQ_MODE_BUFFER: one block
                            of num_samples, DAQ_MODE_PINGPONG: two blocks, written and sent alternately). Can be NULL, daq_init_sampling() creates it
*/
typedef struct {
    uint16_t const num_channels;
    uint16_t const num_samples;
    uint8_t const packet_id;
    uint8_t const packet_tail;
    uint8_t volatile iteration;
    uint64_t volatile runtime_first;
    uint64_t volatile runtime_last;
    bool const is_signed;
    uint8_t const element_size;
    uint8_t const data_type;
    uint8_t volatile send_mode;
    bool volatile new_data;
    daq_buffer_t* buffer;
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


/*! \brief Function to check if the DAQ buffer is full: no sample can be written, because all blocks wait for being sent
* \param data           Pointer to the DAQ data structure
* \return               true if the buffer is full, false otherwise
*/
bool daq_is_fifo_full(daq_data_t* data);


/*! \brief Function to check if the DAQ buffer is empty (no complete block and no sample waiting)
* \param data               Pointer to the DAQ data structure
* \return                   true if the buffer is empty, false otherwise
*/
bool daq_is_empty_fifo(daq_data_t* data);


/*! \brief Function to check if the DAQ data is ready to be sent
* The FIFO of a stream is protected by a spinlock, so a stream can be filled by daq_irq_process() on one core and sent by this function on the other core
* \param data           Pointer to the DAQ data structure
* \return               true if data is ready to be sent, false otherwise
*/
bool daq_check_send_data(daq_data_t* data);


/*! \brief Function to send the DAQ data packet via the selected transport
* \param data           Pointer to the DAQ data packet
*/
void daq_send_data(daq_data_t* data);


/*! \brief Function for processing the data in th DAQ IRQ (also usable from the other core, the FIFO is protected by a spinlock)
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
