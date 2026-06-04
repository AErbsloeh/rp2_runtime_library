#include "hal/daq/double_buffer.h"


volatile fifo_t fifo1;
volatile fifo_t fifo2;


bool double_buffer_init(double_buffer_t* db){
    fifo_init(fifo1);
    fifo_init(fifo2);
    db->write_fifo = &fifo1;
    db->read_fifo = &fifo2;
    db->fifo_overflow = false;
    return true;
};


bool double_buffer_push(double_buffer_t* db, void* data){
    return fifo_push(db->write_fifo, data);
};


bool double_buffer_pop(double_buffer_t* db, void* data){
    return fifo_pop(db->read_fifo, data);
}


bool double_buffer_switch(double_buffer_t* db){      
    if(fifo_is_full(db->write_fifo) && fifo_is_empty(db->read_fifo)){
        fifo_t* temp_fifo = db->write_fifo;
        db->write_fifo = db->read_fifo;
        db->read_fifo = temp_fifo;
        db->fifo_overflow = false;             
        return true;
    }
    else if(fifo_is_full(db->write_fifo) && !fifo_is_empty(db->read_fifo)){
        db->fifo_overflow = true;
        return false;
    }
    else {
        return false;
    }
};

