#ifndef FIFO_H_
#define FIFO_H_


#include "pico/stdlib.h"
#include "pico/util/queue.h"


/*! @brief Structure representing a FIFO buffer
    @param queue         Queue structure from pico-sdk for implementing the FIFO buffer
    @param element_size  Size of each element in the FIFO buffer
    @param length        Maximum number of elements in the FIFO
 */
typedef struct {
    queue_t queue;
    size_t element_size;
    size_t length;
} fifo_t;


/*! @brief Initialize a FIFO buffer
    @param f Pointer to the FIFO buffer structure
    @return true if initialization was successful, false otherwise
 */
bool fifo_init(fifo_t *f);


/*! @brief Push an element into the FIFO buffer
    @param f Pointer to the FIFO buffer structure
    @param data Pointer to the data to be pushed
    @return true if the operation was successful, false otherwise
 */
bool fifo_push(fifo_t *f, void *data);


/*! @brief Push an element into the FIFO buffer (blocking)
    @param f Pointer to the FIFO buffer structure
    @param data Pointer to the data to be pushed
 */
void fifo_push_blocking(fifo_t *f, void *data);


/*! @brief Pop an element from the FIFO buffer
    @param f Pointer to the FIFO buffer structure
    @param data Pointer to the location where the popped data should be stored
    @return true if the operation was successful, false otherwise
 */
bool fifo_pop(fifo_t *f, void *data);


/*! @brief Pop an element from the FIFO buffer (blocking)
    @param f Pointer to the FIFO buffer structure
    @param data Pointer to the location where the popped data should be stored
 */
void fifo_pop_blocking(fifo_t *f, void *data);


/*! @brief Check if the FIFO buffer is empty
    @param f Pointer to the FIFO buffer structure
    @return true if the buffer is empty, false otherwise
 */
bool fifo_is_empty(fifo_t *f);


/*! @brief Check if the FIFO buffer is full
    @param f Pointer to the FIFO buffer structure
    @return true if the buffer is full, false otherwise
 */
bool fifo_is_full(fifo_t *f);


#endif
