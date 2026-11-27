#include "hal/fifo/fifo.h"
#include <stdlib.h>
#include <string.h>
#include "hardware/dma.h"


#define FIFO_DMA_MAX_RING_BYTES 32768u


// ==================== PRIVATE FUNCS ====================//
static size_t fifo_dma_ring_bytes(const fifo_t *f) {
    return f->element_size * f->length;
}


static uint fifo_dma_ring_bits(size_t bytes) {
    uint bits = 0;
    while (((size_t)1u << bits) < bytes) {
        bits++;
    }
    return bits;
}


static bool fifo_dma_config_valid(const fifo_t *f) {
    size_t bytes = fifo_dma_ring_bytes(f);
    bool size_ok = (f->element_size == 1) || (f->element_size == 2) || (f->element_size == 4);
    bool ring_ok = (bytes >= 2) && (bytes <= FIFO_DMA_MAX_RING_BYTES) && ((bytes & (bytes - 1)) == 0);
    return size_ok && ring_ok && (f->dma_source != NULL);
}


static enum dma_channel_transfer_size fifo_dma_transfer_size(const fifo_t *f) {
    switch (f->element_size) {
        case 1:  return DMA_SIZE_8;
        case 2:  return DMA_SIZE_16;
        default: return DMA_SIZE_32;
    }
}


static void fifo_dma_configure_channel(const fifo_t *f, int channel, int chain_to) {
    dma_channel_config cfg = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&cfg, fifo_dma_transfer_size(f));
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_ring(&cfg, true, fifo_dma_ring_bits(fifo_dma_ring_bytes(f)));
    channel_config_set_dreq(&cfg, f->dma_dreq);
    channel_config_set_chain_to(&cfg, chain_to);
    dma_channel_configure(channel, &cfg, f->internal.dma_storage, f->dma_source, f->length, false);
}


static int fifo_dma_active_channel(const fifo_t *f) {
    return dma_channel_is_busy(f->internal.dma_channel[0]) ? f->internal.dma_channel[0] : f->internal.dma_channel[1];
}


static size_t fifo_dma_head(const fifo_t *f) {
    if (!f->internal.dma_running) {
        return f->internal.dma_head_stopped;
    }
    uintptr_t write_addr = (uintptr_t)dma_channel_hw_addr(fifo_dma_active_channel(f))->write_addr;
    return (size_t)(write_addr - (uintptr_t)f->internal.dma_storage);
}


// ==================== CALLABLE FUNCS ====================//
bool fifo_init(fifo_t *f) {
    if (!f->use_dma) {
        queue_init(&f->internal.queue, f->element_size, f->length);
        return true;
    }
    if (!fifo_dma_config_valid(f)) {
        return false;
    }

    size_t bytes = fifo_dma_ring_bytes(f);
    f->internal.dma_storage_raw = (uint8_t *)malloc(2 * bytes);
    if (f->internal.dma_storage_raw == NULL) {
        return false;
    }
    f->internal.dma_storage = (uint8_t *)(((uintptr_t)f->internal.dma_storage_raw + bytes - 1) & ~((uintptr_t)bytes - 1));
    memset(f->internal.dma_storage, 0, bytes);

    f->internal.dma_channel[0] = dma_claim_unused_channel(false);
    f->internal.dma_channel[1] = (f->internal.dma_channel[0] >= 0) ? dma_claim_unused_channel(false) : -1;
    if (f->internal.dma_channel[1] < 0) {
        if (f->internal.dma_channel[0] >= 0) {
            dma_channel_unclaim(f->internal.dma_channel[0]);
        }
        free(f->internal.dma_storage_raw);
        f->internal.dma_storage_raw = NULL;
        f->internal.dma_storage = NULL;
        return false;
    }

    f->internal.dma_tail = 0;
    f->internal.dma_head_stopped = 0;
    f->internal.dma_running = false;
    return true;
}


void fifo_deinit(fifo_t *f) {
    if (!f->use_dma) {
        queue_free(&f->internal.queue);
        return;
    }
    if (f->internal.dma_storage_raw == NULL) {
        return;
    }
    fifo_dma_stop(f);
    dma_channel_unclaim(f->internal.dma_channel[0]);
    dma_channel_unclaim(f->internal.dma_channel[1]);
    free(f->internal.dma_storage_raw);
    f->internal.dma_storage_raw = NULL;
    f->internal.dma_storage = NULL;
}


bool fifo_dma_start(fifo_t *f) {
    if (!f->use_dma || f->internal.dma_storage == NULL) {
        return false;
    }
    fifo_dma_stop(f);

    fifo_dma_configure_channel(f, f->internal.dma_channel[0], f->internal.dma_channel[1]);
    fifo_dma_configure_channel(f, f->internal.dma_channel[1], f->internal.dma_channel[0]);
    f->internal.dma_tail = 0;
    f->internal.dma_head_stopped = 0;
    f->internal.dma_running = true;
    dma_channel_start(f->internal.dma_channel[0]);
    return true;
}


void fifo_dma_stop(fifo_t *f) {
    if (!f->use_dma || !f->internal.dma_running) {
        return;
    }
    int active = fifo_dma_active_channel(f);
    uint32_t mask = (1u << f->internal.dma_channel[0]) | (1u << f->internal.dma_channel[1]);
    dma_hw->abort = mask;
    while (dma_hw->abort & mask) {
        tight_loop_contents();
    }
    f->internal.dma_head_stopped = (size_t)((uintptr_t)dma_channel_hw_addr(active)->write_addr - (uintptr_t)f->internal.dma_storage);
    f->internal.dma_running = false;
}


bool fifo_push(fifo_t *f, void *data) {
    if (f->use_dma) {
        return false;
    }
    return queue_try_add(&f->internal.queue, data);
}


void fifo_push_blocking(fifo_t *f, void *data) {
    if (f->use_dma) {
        return;
    }
    queue_add_blocking(&f->internal.queue, data);
}


bool fifo_pop(fifo_t *f, void *data) {
    if (!f->use_dma) {
        return queue_try_remove(&f->internal.queue, data);
    }
    if (fifo_is_empty(f)) {
        return false;
    }
    memcpy(data, &f->internal.dma_storage[f->internal.dma_tail], f->element_size);
    f->internal.dma_tail = (f->internal.dma_tail + f->element_size) & (fifo_dma_ring_bytes(f) - 1);
    return true;
}


void fifo_pop_blocking(fifo_t *f, void *data) {
    if (!f->use_dma) {
        queue_remove_blocking(&f->internal.queue, data);
        return;
    }
    while (!fifo_pop(f, data)) {
        tight_loop_contents();
    }
}


size_t fifo_get_level(fifo_t *f) {
    if (!f->use_dma) {
        return queue_get_level(&f->internal.queue);
    }
    if (f->internal.dma_storage == NULL) {
        return 0;
    }
    size_t used_bytes = (fifo_dma_head(f) - f->internal.dma_tail) & (fifo_dma_ring_bytes(f) - 1);
    return used_bytes / f->element_size;
}


bool fifo_is_empty(fifo_t *f) {
    if (!f->use_dma) {
        return queue_is_empty(&f->internal.queue);
    }
    return fifo_get_level(f) == 0;
}


bool fifo_is_full(fifo_t *f) {
    if (!f->use_dma) {
        return queue_is_full(&f->internal.queue);
    }
    return fifo_get_level(f) >= (f->length - 1);
}
