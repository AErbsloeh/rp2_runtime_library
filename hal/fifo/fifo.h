#ifndef FIFO_H_
#define FIFO_H_


#include "pico/stdlib.h"
#include "pico/util/queue.h"


/*! @brief Internal state of a FIFO buffer, it is set by the functions of the FIFO and has to be left untouched by the user
    It is zero after the definition of the FIFO (static variables and initializers with designated members set the rest to 0)
    @param queue            Queue structure from pico-sdk for implementing the FIFO buffer (not used with DMA)
    @param dma_storage      Aligned memory of the ring buffer (DMA only)
    @param dma_storage_raw  Memory which was allocated for the ring buffer (DMA only)
    @param dma_channel      The two chained DMA channels (DMA only)
    @param dma_tail         Byte offset of the next element which is popped (DMA only)
    @param dma_head_stopped Byte offset of the write position at the time the DMA was stopped (DMA only)
    @param dma_running      true while the DMA writes into the buffer (DMA only)
 */
typedef struct {
    queue_t queue;
    uint8_t *dma_storage;
    uint8_t *dma_storage_raw;
    int dma_channel[2];
    size_t dma_tail;
    size_t dma_head_stopped;
    bool dma_running;
} fifo_internal_t;


/*! @brief Structure representing a FIFO buffer
    The FIFO is either a thread-safe queue of the pico-sdk (use_dma = false) or a sampling buffer which is filled by the DMA
    (use_dma = true): two chained DMA channels write the data of a peripheral register (e.g. ADC, SPI, PIO) endlessly into a
    ring buffer, without any load for the CPU, the software only pops the elements.
    The DMA mode is limited to one consumer, element_size of 1, 2 or 4 bytes and element_size * length being a power of two
    (2 .. 32768 bytes). If the consumer is slower than the peripheral, the oldest elements are overwritten (see fifo_is_full).
    @param element_size  Size of each element in the FIFO buffer
    @param length        Maximum number of elements in the FIFO
    @param use_dma       true: sampling buffer written by the DMA, false: queue (default)
    @param dma_source    DMA only: address of the peripheral register to read from (e.g. &adc_hw->fifo)
    @param dma_dreq      DMA only: transfer request signal which paces the transfers (e.g. DREQ_ADC, spi_get_dreq(spi0, false))
    @param internal      Internal state, do not set or change it
 */
typedef struct {
    size_t element_size;
    size_t length;
    bool use_dma;
    const volatile void *dma_source;
    uint32_t dma_dreq;
    fifo_internal_t internal;
} fifo_t;


/*! @brief Initialize a FIFO buffer
    With DMA the memory and the two DMA channels are reserved, the transfer is started with fifo_dma_start()
    @param f Pointer to the FIFO buffer structure
    @return true if initialization was successful, false otherwise (DMA: invalid configuration, no free DMA channel or memory)
 */
bool fifo_init(fifo_t *f);


/*! @brief Release a FIFO buffer (stops the DMA, frees the DMA channels and the memory)
    @param f Pointer to the FIFO buffer structure
 */
void fifo_deinit(fifo_t *f);


/*! @brief Start the DMA which fills the FIFO buffer (the buffer is empty afterwards), the peripheral has to be started separately
    @param f Pointer to the FIFO buffer structure
    @return true if the DMA is running, false if the FIFO does not use the DMA or is not initialized
 */
bool fifo_dma_start(fifo_t *f);


/*! @brief Stop the DMA, elements which are already in the FIFO buffer can still be popped
    @param f Pointer to the FIFO buffer structure
 */
void fifo_dma_stop(fifo_t *f);


/*! @brief Push an element into the FIFO buffer
    @param f Pointer to the FIFO buffer structure
    @param data Pointer to the data to be pushed
    @return true if the operation was successful, false otherwise (always false with DMA, the DMA is the only producer)
 */
bool fifo_push(fifo_t *f, void *data);


/*! @brief Push an element into the FIFO buffer (blocking)
    @param f Pointer to the FIFO buffer structure
    @param data Pointer to the data to be pushed
    @note Without effect with DMA, the DMA is the only producer
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
    @note With DMA this waits until the DMA has written the next element, so it never returns if the DMA does not run
 */
void fifo_pop_blocking(fifo_t *f, void *data);


/*! @brief Get the number of elements in the FIFO buffer
    @param f Pointer to the FIFO buffer structure
    @return Number of elements which can be popped
 */
size_t fifo_get_level(fifo_t *f);


/*! @brief Check if the FIFO buffer is empty
    @param f Pointer to the FIFO buffer structure
    @return true if the buffer is empty, false otherwise
 */
bool fifo_is_empty(fifo_t *f);


/*! @brief Check if the FIFO buffer is full
    @param f Pointer to the FIFO buffer structure
    @return true if the buffer is full, false otherwise. With DMA the buffer is full if only one element is free, the DMA
            overwrites the oldest elements if the consumer does not pop fast enough (it cannot be stopped)
 */
bool fifo_is_full(fifo_t *f);


#endif
