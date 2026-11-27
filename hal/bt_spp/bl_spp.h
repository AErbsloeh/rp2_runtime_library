#ifndef BTSTACK_SPP_H_
#define BTSTACK_SPP_H_


#include "pico/stdlib.h"


#ifndef BTSTACK_SPP_SEND_TIMEOUT_MS
#define BTSTACK_SPP_SEND_TIMEOUT_MS 1000
#endif


/*! \brief Callback function for processing data received from the connected Bluetooth device
 *  The function is called from the BTstack context (background of the CYW43 async context), not from the main loop, so it has to be short
 *  \param context  Pointer to user-defined callback context
 *  \param data     Pointer to received data buffer
 *  \param len      Length of received data buffer
 */
typedef void (*btstack_spp_recv_cb_t)(void *context, const uint8_t *data, size_t len);


/*! \brief Struct handler for configuring the Bluetooth Classic SPP (serial port profile) server of the CYW43 (Pico W, Pico 2 W, Raspberry Pi Radio Module 2)
 *  The remote device (host computer, smartphone) pairs with the RP2xxx and opens the virtual serial port, which is used like the USB serial port
 *  \param init_done            Boolean for initialization state
 *  \param name                 Name of the device which is visible during the search (BTstack replaces 00:00:00:00:00:00 with the address of the device)
 *  \param pin                  PIN for pairing of remote devices without secure simple pairing (NULL: "0000")
 *  \param rfcomm_channel       RFCOMM server channel of the service (0: 1)
 *  \param on_receive           Callback function for received data
 *  \param on_receive_context   User-defined context passed to receive callback
 *  \param connected            Boolean (set by the module) if a remote device has opened the serial port
 *  \param rfcomm_cid           RFCOMM channel id of the open connection (set by the module)
 *  \param max_frame_size       Maximum number of bytes per RFCOMM packet of the open connection (set by the module)
 */
typedef struct {
    bool init_done;
    const char *name;
    const char *pin;
    uint8_t rfcomm_channel;

    btstack_spp_recv_cb_t on_receive;
    void *on_receive_context;

    volatile bool connected;
    uint16_t rfcomm_cid;
    uint16_t max_frame_size;
} btstack_spp_t;


/*! \brief  Function for initialising the CYW43 and the BTstack and for starting the Bluetooth Classic SPP server
 *  The device is discoverable and connectable afterwards, the BTstack runs in the background of the CYW43 async context
 *  The CYW43 is only initialised if the application has not done it before (e.g. for the LED of the Pico W)
 *  Only one handler (one server) is supported
 *  \param handler  Pointer to the handler (the struct has to stay valid while the server runs)
 *  \return         Boolean if the server is started
 */
bool btstack_spp_init(btstack_spp_t *handler);


/*! \brief  Function for stopping the SPP server and switching off the Bluetooth controller
 *  \param handler  Pointer to the handler
 */
void btstack_spp_deinit(btstack_spp_t *handler);


/*! \brief  Function for checking if a remote device has opened the serial port
 *  \param handler  Pointer to the handler
 *  \return         Boolean if a connection is open
 */
bool btstack_spp_is_connected(const btstack_spp_t *handler);


/*! \brief  Function for sending bytes to the connected remote device, it waits until the data are handed to the Bluetooth controller
 *  The wait ends after BTSTACK_SPP_SEND_TIMEOUT_MS (default 1000 ms) without progress or if the connection is closed
 *  \param handler  Pointer to the handler
 *  \param data     Buffer with bytes to send
 *  \param len      Number of bytes to send
 *  \return         Number of sent bytes (0 without connection)
 */
size_t btstack_spp_send(btstack_spp_t *handler, const uint8_t *data, size_t len);


#endif
