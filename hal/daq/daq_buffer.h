#ifndef DAQ_BUFFER_H_
#define DAQ_BUFFER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define DAQ_BUFFER_MAX_BLOCKS 2u


/*! \brief Structure representing one block of a DAQ buffer
    \param data             Pointer to the storage of the block (inside of the storage of the buffer)
    \param capacity_bytes   Available storage capacity of the block in bytes
    \param sample_count     Number of complete samples currently stored
    \param ready            Flag indicating that the block is complete and waits for being read
    \param timestamp_first  Timestamp of the first sample in the block
    \param timestamp_last   Timestamp of the last sample in the block
*/
typedef struct {
    uint8_t *data;
    size_t capacity_bytes;

    volatile uint16_t sample_count;
    volatile bool ready;

    uint64_t timestamp_first;
    uint64_t timestamp_last;
} daq_buffer_block_t;


/*! \brief Structure representing the buffer of a DAQ stream, a single-producer/single-consumer ring of one or two blocks
    The block buffer is used by all DAQ modes: one sample per block (DAQ_MODE_SAMPLE), one block with all samples (DAQ_MODE_BUFFER)
    and two blocks which are written and sent alternately (DAQ_MODE_PINGPONG, ping-pong)
    \param blocks             Blocks of the buffer (only the first num_blocks are used)
    \param storage            Memory of all blocks, created and enlarged by daq_buffer_init()
    \param storage_bytes      Size of the memory in bytes
    \param num_blocks         Number of used blocks (1 .. DAQ_BUFFER_MAX_BLOCKS)
    \param samples_per_block  Number of complete samples stored in each block
    \param bytes_per_sample   Number of bytes occupied by one complete sample (all channels)
    \param write_index        Index of the block currently used by the producer
    \param read_index         Index of the block currently used by the consumer
    \param num_dropped        Number of samples which were dropped because all blocks waited for being read (consumer subtracts what it handled)
*/
typedef struct {
    daq_buffer_block_t blocks[DAQ_BUFFER_MAX_BLOCKS];
    uint8_t *storage;
    size_t storage_bytes;

    uint8_t num_blocks;
    uint16_t samples_per_block;
    size_t bytes_per_sample;

    volatile uint8_t write_index;
    volatile uint8_t read_index;

    volatile uint32_t num_dropped;
} daq_buffer_t;


/*! \brief Initialize the buffer, the memory is created at the first call and only enlarged if a later call needs more
    The caller has to make sure that producer and consumer do not use the buffer while it is initialized
    \param buffer              Pointer to the buffer (zero-initialized at the first call)
    \param num_blocks          Number of blocks (1 .. DAQ_BUFFER_MAX_BLOCKS)
    \param samples_per_block   Number of complete samples per block
    \param bytes_per_sample    Number of bytes occupied by one complete sample
    \return                    true if initialization was successful, false otherwise
*/
bool daq_buffer_init(daq_buffer_t *buffer, uint8_t num_blocks, uint16_t samples_per_block, size_t bytes_per_sample);


/*! \brief Reset all blocks without releasing the memory
    \param buffer  Pointer to the buffer
*/
void daq_buffer_reset(daq_buffer_t *buffer);


/*! \brief Append one complete sample (all channels) to the block of the producer
    \param buffer     Pointer to the buffer
    \param sample     Pointer to one complete sample
    \param timestamp  Timestamp associated with the sample
    \return           true if the sample was accepted, false if no block was writable (all blocks wait for being read, the sample is counted in num_dropped)
*/
bool daq_buffer_push_sample(daq_buffer_t *buffer, const void *sample, uint64_t timestamp);


/*! \brief Get the next complete block for reading
    \param buffer  Pointer to the buffer
    \return        Pointer to the complete block, or NULL if no block is ready
*/
const daq_buffer_block_t* daq_buffer_get_read_block(const daq_buffer_t *buffer);


/*! \brief Release the block of daq_buffer_get_read_block() and switch to the next block
    \param buffer  Pointer to the buffer
    \return        true if a ready block was released, false otherwise
*/
bool daq_buffer_release_read(daq_buffer_t *buffer);


/*! \brief Check if the producer has no block to write because all blocks wait for being read
    \param buffer  Pointer to the buffer
    \return        true if no sample can be pushed, false otherwise
*/
bool daq_buffer_is_full(const daq_buffer_t *buffer);


/*! \brief Check if the buffer holds no sample (no complete block and no sample in the block of the producer)
    \param buffer  Pointer to the buffer
    \return        true if the buffer is empty, false otherwise
*/
bool daq_buffer_is_empty(const daq_buffer_t *buffer);


#endif
