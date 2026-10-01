#include "hal/daq/ping_pong_buffer.h"

#include <stdlib.h>
#include <string.h>

static void ping_pong_block_reset(ping_pong_block_t *block) {
    block->sample_count = 0;
    block->timestamp_first = 0;
    block->timestamp_last = 0;
    block->ready = false;
}

void ping_pong_buffer_reset(
    ping_pong_buffer_t *buffer
) {
    if (buffer == NULL) {
        return;
    }

    for (uint8_t index = 0; index < PING_PONG_NUM_BLOCKS; index++) {
        ping_pong_block_reset(&buffer->blocks[index]);
    }
    buffer->write_index = 0;
    buffer->read_index = 0;
}

bool ping_pong_buffer_init(
    ping_pong_buffer_t *buffer,
    uint16_t samples_per_block,
    size_t bytes_per_sample
) {
    if (
        buffer == NULL || 
        samples_per_block == 0 || 
        bytes_per_sample == 0
    ) {
        return false;
    }

    if (bytes_per_sample > SIZE_MAX / samples_per_block) {
        return false;
    }

    size_t required_capacity = samples_per_block * bytes_per_sample;

    if (buffer->blocks[0].data == NULL && buffer->blocks[1].data == NULL) {
        if (required_capacity > SIZE_MAX / PING_PONG_NUM_BLOCKS) {
            return false;
        }

        uint8_t *storage = malloc(required_capacity * PING_PONG_NUM_BLOCKS);

        if (storage == NULL) {
            return false;
        }

        for (uint8_t index = 0; index < PING_PONG_NUM_BLOCKS; index++) {
            buffer->blocks[index].data = storage + index * required_capacity;
            buffer->blocks[index].capacity_bytes = required_capacity;
        }
    }

    for (uint8_t index = 0; index < PING_PONG_NUM_BLOCKS; index++) {
        if (
            buffer->blocks[index].data == NULL || 
            buffer->blocks[index].capacity_bytes < required_capacity
        ) {
            return false;
        }
    }

    buffer->samples_per_block = samples_per_block;
    buffer->bytes_per_sample = bytes_per_sample;

    ping_pong_buffer_reset(buffer);
    return true;
}


bool ping_pong_buffer_push_sample(
    ping_pong_buffer_t *buffer, 
    const void *sample, 
    uint64_t timestamp
) {
    if (
        buffer == NULL || 
        sample == NULL || 
        buffer->write_index >= PING_PONG_NUM_BLOCKS || 
        buffer->samples_per_block == 0 || 
        buffer->bytes_per_sample == 0
    ) {
        return false;
    }

    ping_pong_block_t *block = &buffer->blocks[buffer->write_index];

    if (
        block->ready || 
        block->data == NULL || 
        block->sample_count >= buffer->samples_per_block
    ) {
        return false;
    }

    size_t offset = block->sample_count * buffer->bytes_per_sample;

    if (
        offset > block->capacity_bytes || 
        buffer->bytes_per_sample > block->capacity_bytes - offset
    ) {
        return false;
    }

    if (block->sample_count == 0) {
        block->timestamp_first = timestamp;
    }

    memcpy(
        &block->data[offset],
        sample,
        buffer->bytes_per_sample
    );

    block->sample_count++;
    block->timestamp_last = timestamp;

    if (block->sample_count == buffer->samples_per_block) {
        block->ready = true;
        buffer->write_index ^= 1u;
    }

    return true;

}


const ping_pong_block_t* ping_pong_buffer_get_read_block(
    const ping_pong_buffer_t *buffer
) {
    if (
        buffer == NULL ||
        buffer->read_index >= PING_PONG_NUM_BLOCKS
    ) {
        return NULL;
    }

    const ping_pong_block_t *block = &buffer->blocks[buffer->read_index];

    if (!block->ready) {
        return NULL;
    }

    return block;
}


bool ping_pong_buffer_release_read(
    ping_pong_buffer_t *buffer
) {
    if (
        buffer == NULL || 
        buffer->read_index >= PING_PONG_NUM_BLOCKS
    ) {
        return false;
    }

    ping_pong_block_t *block = &buffer->blocks[buffer->read_index];

    if (!block->ready) {
        return false;
    }

    ping_pong_block_reset(block);
    buffer->read_index ^= 1u;

    return true;
}
