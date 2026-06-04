#ifndef DOUBLE_BUFFER_H_
#define DOUBLE_BUFFER_H_
#include "hal/daq/fifo.h"


/*! @brief Structure representing a double buffer for DAQ sampling
    @param write_fifo    Pointer to the current write FIFO
    @param read_fifo     Pointer to the current read FIFO
    @param fifo_overflow Flag indicating if a FIFO overflow has occurred
 */
typedef struct {
    fifo_t* write_fifo;
    fifo_t* read_fifo;
    bool fifo_overflow;
} double_buffer_t;


/*! @brief Initialize double buffer
    @param db Pointer to the double buffer structure
    @return true if initialization was successful, false otherwise
 */
bool double_buffer_init(double_buffer_t* db);


/*! @brief Push an element into the current write FIFO of the double buffer
    @param db Pointer to the double buffer structure
    @param data Pointer to the data to be pushed
    @return true if the operation was successful, false otherwise
 */
bool double_buffer_push(double_buffer_t* db, void* data);


/*! @brief Switch the write and read FIFOs of the double buffer
    @param db Pointer to the double buffer structure
    @return true if the operation was successful, false otherwise
 */
bool double_buffer_switch(double_buffer_t* db);


/*! @brief Pop an element from the current read FIFO of the double buffer 
    @param db Pointer to the double buffer structure    
    @param f Pointer to the FIFO buffer structure
    @return true if the operation was successful, false otherwise
 */
bool double_buffer_pop(double_buffer_t* db, void* data);


#endif