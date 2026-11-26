#include "hal/daq/daq_buffer.h"

#include <stdlib.h>
#include <string.h>


static void daq_buffer_block_reset(daq_buffer_block_t *block){
    block->sample_count = 0;
    block->timestamp_first = 0;
    block->timestamp_last = 0;
    block->ready = false;
}


void daq_buffer_reset(daq_buffer_t *buffer){
    if(buffer == NULL){
        return;
    }
    for(uint8_t index = 0; index < DAQ_BUFFER_MAX_BLOCKS; index++){
        daq_buffer_block_reset(&buffer->blocks[index]);
    }
    buffer->write_index = 0;
    buffer->read_index = 0;
    buffer->num_dropped = 0;
}


bool daq_buffer_init(daq_buffer_t *buffer, uint8_t num_blocks, uint16_t samples_per_block, size_t bytes_per_sample){
    if((buffer == NULL) || (num_blocks == 0) || (num_blocks > DAQ_BUFFER_MAX_BLOCKS) || (samples_per_block == 0) || (bytes_per_sample == 0)){
        return false;
    }
    if(bytes_per_sample > SIZE_MAX / samples_per_block){
        return false;
    }
    const size_t block_bytes = (size_t)samples_per_block * bytes_per_sample;
    if(block_bytes > SIZE_MAX / num_blocks){
        return false;
    }
    const size_t storage_bytes = block_bytes * num_blocks;

    if((buffer->storage == NULL) || (buffer->storage_bytes < storage_bytes)){
        uint8_t *storage = malloc(storage_bytes);
        if(storage == NULL){
            return false;
        }
        free(buffer->storage);
        buffer->storage = storage;
        buffer->storage_bytes = storage_bytes;
    }

    for(uint8_t index = 0; index < DAQ_BUFFER_MAX_BLOCKS; index++){
        if(index < num_blocks){
            buffer->blocks[index].data = buffer->storage + (index * block_bytes);
            buffer->blocks[index].capacity_bytes = block_bytes;
        } else {
            buffer->blocks[index].data = NULL;
            buffer->blocks[index].capacity_bytes = 0;
        }
    }
    buffer->num_blocks = num_blocks;
    buffer->samples_per_block = samples_per_block;
    buffer->bytes_per_sample = bytes_per_sample;

    daq_buffer_reset(buffer);
    return true;
}


bool daq_buffer_push_sample(daq_buffer_t *buffer, const void *sample, uint64_t timestamp){
    if((buffer == NULL) || (sample == NULL) || (buffer->num_blocks == 0) || (buffer->write_index >= buffer->num_blocks)){
        return false;
    }
    daq_buffer_block_t *block = &buffer->blocks[buffer->write_index];
    if(block->ready){
        buffer->num_dropped++;
        return false;
    }
    if((block->data == NULL) || (block->sample_count >= buffer->samples_per_block)){
        return false;
    }
    const size_t offset = (size_t)block->sample_count * buffer->bytes_per_sample;
    if((offset > block->capacity_bytes) || (buffer->bytes_per_sample > block->capacity_bytes - offset)){
        return false;
    }

    if(block->sample_count == 0){
        block->timestamp_first = timestamp;
    }
    memcpy(&block->data[offset], sample, buffer->bytes_per_sample);
    block->sample_count++;
    block->timestamp_last = timestamp;

    if(block->sample_count == buffer->samples_per_block){
        block->ready = true;
        buffer->write_index = (uint8_t)((buffer->write_index + 1u) % buffer->num_blocks);
    }
    return true;
}


const daq_buffer_block_t* daq_buffer_get_read_block(const daq_buffer_t *buffer){
    if((buffer == NULL) || (buffer->num_blocks == 0) || (buffer->read_index >= buffer->num_blocks)){
        return NULL;
    }
    const daq_buffer_block_t *block = &buffer->blocks[buffer->read_index];
    return block->ready ? block : NULL;
}


bool daq_buffer_release_read(daq_buffer_t *buffer){
    if((buffer == NULL) || (buffer->num_blocks == 0) || (buffer->read_index >= buffer->num_blocks)){
        return false;
    }
    daq_buffer_block_t *block = &buffer->blocks[buffer->read_index];
    if(!block->ready){
        return false;
    }
    daq_buffer_block_reset(block);
    buffer->read_index = (uint8_t)((buffer->read_index + 1u) % buffer->num_blocks);
    return true;
}


bool daq_buffer_is_full(const daq_buffer_t *buffer){
    if((buffer == NULL) || (buffer->num_blocks == 0) || (buffer->write_index >= buffer->num_blocks)){
        return true;
    }
    return buffer->blocks[buffer->write_index].ready;
}


bool daq_buffer_is_empty(const daq_buffer_t *buffer){
    if((buffer == NULL) || (buffer->num_blocks == 0) || (buffer->write_index >= buffer->num_blocks)){
        return true;
    }
    return (daq_buffer_get_read_block(buffer) == NULL) && (buffer->blocks[buffer->write_index].sample_count == 0);
}
