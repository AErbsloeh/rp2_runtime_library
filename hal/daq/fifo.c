#include "hal/daq/fifo.h"


// ==================== CALLABLE FUNCS ====================//
bool fifo_init(fifo_t *f) {
    queue_init(&f->queue, f->element_size, f->length);
    return true;
}


bool fifo_push(fifo_t *f, void *data){
    return queue_try_add(&f->queue, data);
}


void fifo_push_blocking(fifo_t *f, void *data){
    queue_add_blocking(&f->queue, data);
}


bool fifo_pop(fifo_t *f, void *data) {
    return queue_try_remove(&f->queue, data);
}


void fifo_pop_blocking(fifo_t *f, void *data) {
    queue_remove_blocking(&f->queue, data);
}


bool fifo_is_empty(fifo_t *f) {
    return queue_is_empty(&f->queue);
}


bool fifo_is_full(fifo_t *f) {
    return queue_is_full(&f->queue);
}
