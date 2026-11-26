#ifndef PROCESSING_H_
#define PROCESSING_H_


#include "pico/stdlib.h"
#include "hal/daq/daq.h"


/*! \brief Layout of one data block which is exchanged with core 1 (same order as in the DAQ streams: samples one after another, channels interleaved)
    \param num_channels     Number of channels
    \param num_samples      Number of samples per channel in one block
    \param element_size     Number of bytes of one value (MSB first for PROCESS_DATA)
*/
typedef struct {
    uint16_t num_channels;
    uint16_t num_samples;
    uint8_t element_size;
} processing_shape_t;


/*! \brief Origin of the data blocks which go through the processing (the results of all origins go into the same DAQ stream)
    \param PROCESSING_SOURCE_HOST       Data from the computer (USB/WiFi, test operation, command PROCESS_DATA)
    \param PROCESSING_SOURCE_SENSOR     Data from the sensors (normal operation, fed from the DAQ timer interrupt)
*/
typedef enum {
    PROCESSING_SOURCE_HOST = 0,
    PROCESSING_SOURCE_SENSOR,
    PROCESSING_NUM_SOURCES
} processing_source_t;


struct processing_config;


/*! \brief Function type of the user logic, which processes one data block on core 1
* \param config     Pointer to the settings (layout of the input and output block)
* \param in         Pointer to the input block (processing_get_num_bytes(&config->in) bytes), do not modify
* \param out        Pointer to the buffer for the output block (processing_get_num_bytes(&config->out) bytes)
* \return           True if the result in out is valid and shall be written into the DAQ stream, false for no result
*/
typedef bool (*processing_func_t)(const struct processing_config *config, const uint8_t *in, uint8_t *out);


/*! \brief Settings of the data processing on core 1
    \param in               Layout of the data block which goes into the processing
    \param out              Layout of the data block which comes out of the processing
    \param queue_depth      Number of data blocks which fit into the queue to core 1 (per origin)
    \param daq_out          DAQ stream which gets the results, has to match the output layout (num_channels and element_size; the stream collects blocks, so its num_samples has to be a multiple of the num_samples of the output block, or the output block has one sample if the stream sends single samples). Core 1 writes into it, so core 0 only has to send it with daq_check_send_data()
    \param func             User logic, which is called on core 1 for each data block
*/
typedef struct processing_config {
    processing_shape_t in;
    processing_shape_t out;
    uint8_t queue_depth;
    daq_data_t *daq_out;
    processing_func_t func;
} processing_config_t;


/*! \brief Number of bytes of one data block
* \param shape      Pointer to the layout of the data block
* \return           num_channels * num_samples * element_size
*/
size_t processing_get_num_bytes(const processing_shape_t *shape);


/*! \brief Checks the settings (also the layout of the DAQ stream) and initialises the queues to core 1, the DAQ streams have to be initialised before with daq_init_sampling()
* \param config     Pointer to the settings, has to stay valid while the processing runs
* \return           True if the initialisation was successful, false otherwise (also if the processing is already running)
*/
bool processing_init(const processing_config_t *config);


/*! \brief Starts the processing on core 1 (processing_init() has to be called before)
* \return           True if core 1 was started, false if processing_init() was not successful or the processing is already running
*/
bool processing_start(void);


/*! \brief Settings which were handed over to processing_init()
* \return           Pointer to the settings, NULL if processing_init() was not successful
*/
const processing_config_t* processing_get_config(void);


/*! \brief Hands over one data block to core 1 (non-blocking, call it from core 0 only, also usable in an interrupt)
* \param source     Origin of the data block (host or sensors, the blocks of the origins are processed in turns)
* \param in         Pointer to the input block (processing_get_num_bytes(&config->in) bytes)
* \return           True if the block was queued, false if the queue is full (block is dropped)
*/
bool processing_submit(processing_source_t source, const uint8_t *in);


/*! \brief Hands over one sample of the sensors (num_channels * element_size bytes of the input layout), usable in an interrupt
*          The samples are collected until a block of num_samples is complete, then it is handed over like processing_submit() does
* \param sample     Pointer to the sample
* \param length     Number of bytes of the sample, has to match the input layout
* \return           True if the sample was accepted, false if the length does not match or a completed block was dropped
*/
bool processing_push_sample(const uint8_t *sample, size_t length);


/*! \brief Number of blocks which were dropped because the queue to core 1 was full (core 1 waits for a free DAQ stream and the queue fills up) */
uint32_t processing_get_num_dropped(void);


#endif
