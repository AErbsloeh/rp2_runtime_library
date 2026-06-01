#ifndef HAL_TRANSPORT_H_
#define HAL_TRANSPORT_H_

#include "stddef.h"
#include "pico/stdlib.h"

// ============================== DEFINITIONS ==============================
/*! \brief Struct handler for receiving transport data
* \param ready      Boolean if RX buffer has new data ready for processing
* \param length     Length of the used RX buffer
* \param position   Actual position for writing new data into the RX buffer
* \param data       RX data buffer
*/
typedef struct {
    bool ready;
    uint8_t length;
    uint8_t position;
    char *data;
} transport_rx_buffer_t;

// ========================= FUNCTIONS FOR PROCESSING ===========================
/*! \brief Function to initialize the selected transport interface
* \param rx_buffer  Struct handler for handling the transport RX buffer
* \return           Boolean for initialization is successful
*/
bool transport_init(transport_rx_buffer_t *rx_buffer);

/*! \brief Function to wait until the selected transport interface is connected
* \return           Boolean for transport interface is connected
*/
bool transport_wait_until_connected(void);

/*! \brief Function for polling and handling received transport data
* \param rx_buffer  Struct handler for handling the transport RX buffer
*/
void transport_poll_rx(transport_rx_buffer_t *rx_buffer);

/*! \brief Function for sending bytes via the selected transport interface
* \param data       Buffer with bytes to send
* \param len        Number of bytes to send
* \return           Number of written bytes
*/
size_t transport_write(char *data, size_t len);

#endif