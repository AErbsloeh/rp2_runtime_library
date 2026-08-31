#ifndef PING_PONG_BUFFER_H_
#define PING_PONG_BUFFER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define PING_PONG_NUM_BLOCKS 2u

/*! \brief Structure representing one block of a ping-pong buffer
    \param data             Pointer to the externally allocated block storage
    \param capacity_bytes   Available storage capacity in bytes
    \param sample_count     Number of complete samples currently stored
    \param ready            Flag indicating that the block is complete and ready
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
} ping_pong_block_t;


/*! \brief Structure representing a single-producer/single-consumer ping-pong buffer
    \param blocks             Two alternating data blocks
    \param samples_per_block  Number of complete samples stored in each block
    \param bytes_per_sample   Number of bytes occupied by one complete sample
    \param write_index        Index of the block currently used by the producer
    \param read_index         Index of the block currently used by the consumer
*/
typedef struct {
    ping_pong_block_t blocks[PING_PONG_NUM_BLOCKS];

    uint16_t samples_per_block;
    size_t bytes_per_sample;

    volatile uint8_t write_index;
    volatile uint8_t read_index;
} ping_pong_buffer_t;


/*! \brief Initialize and validate a ping-pong buffer
    \param buffer              Pointer to the ping-pong buffer
    \param samples_per_block   Number of complete samples per block
    \param bytes_per_sample    Number of bytes occupied by one complete sample
    \return                    true if initialization was successful, false otherwise
*/
bool ping_pong_buffer_init(
    ping_pong_buffer_t *buffer,
    uint16_t samples_per_block,
    size_t bytes_per_sample
);


/*! \brief Reset both blocks without releasing their storage
    \param buffer  Pointer to the ping-pong buffer
*/
void ping_pong_buffer_reset(
    ping_pong_buffer_t *buffer
);


/*! \brief Append one complete sample to the active write block
    \param buffer     Pointer to the ping-pong buffer
    \param sample     Pointer to one complete sample containing all channels
    \param timestamp  Timestamp associated with the sample
    \return           true if the sample was accepted, false if no block was writable
*/
bool ping_pong_buffer_push_sample(
    ping_pong_buffer_t *buffer, 
    const void *sample, 
    uint64_t timestamp
);


/*! \brief Get the current complete read block
    \param buffer  Pointer to the ping-pong buffer
    \return        Pointer to the complete read block, or NULL if no block is ready
*/
const ping_pong_block_t* ping_pong_buffer_get_read_block(
    const ping_pong_buffer_t *buffer
);


/*! \brief Release the current read block and switch to the other block
    \param buffer  Pointer to the ping-pong buffer
    \return        true if a ready block was released, false otherwise
*/
bool ping_pong_buffer_release_read(
    ping_pong_buffer_t *buffer
);

#endif
