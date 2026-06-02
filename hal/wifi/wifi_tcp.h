
#ifndef WIFI_HANDLER_H_
#define WIFI_HANDLER_H_

#include "pico/stdlib.h"
#include "lwip/tcp.h"


/*! \brief Callback function for processing TCP data received from a connected client
 *  \param context Pointer to user-defined callback context
 *  \param data    Pointer to received data buffer
 *  \param len     Length of received data buffer
 */
typedef void (*wifi_tcp_recv_cb_t)(void *context, const uint8_t *data, size_t len);


/*! \brief Struct handler for configuring WiFi TCP communication between RP2xxx and host computer
 *  \param init_done          Boolean for initialization state
 *  \param use_dhcp           Boolean for using DHCP instead of static IP configuration
 *  \param country            CYW43 country code for WiFi configuration
 *  \param port               TCP server port number
 *  \param hostname           Optional hostname for the network interface
 *  \param ssid               String with SSID from host computer
 *  \param password           String with password for SSID
 *  \param static_ip          Static IPv4 address used when DHCP is disabled
 *  \param netmask            Static IPv4 netmask used when DHCP is disabled
 *  \param gateway            Static IPv4 gateway used when DHCP is disabled
 *  \param on_receive         Callback function for received TCP data
 *  \param on_receive_context User-defined context passed to receive callback
 */
typedef struct {
    bool init_done;
    bool use_dhcp;
    uint32_t country;
    uint16_t port;
    const char* hostname;
    const char* ssid;
    const char* password;

    //only necessary when disabling DHCP
    const char* static_ip;
    const char* netmask;
    const char* gateway;

    wifi_tcp_recv_cb_t on_receive;
    void *on_receive_context;
} wifi_handler_t;


/*! \brief Initializes the WLAN module CYW43 and starts a TCP server
 *  \param handler Pointer to wifi_handler_t struct
 *  \return        Boolean indicating success of initialization
 */
bool wifi_handler_init(wifi_handler_t* handler);


/*! \brief Polling and handling CYW43/lwIP events for TCP communication
 *  \param handler Pointer to wifi_handler_t struct
 */
void wifi_handler_poll(wifi_handler_t* handler);


/*! \brief Checking if a TCP client is connected
 *  \param handler Pointer to wifi_handler_t struct
 *  \return        Boolean indicating if a TCP client is connected
 */
bool wifi_handler_has_clients(wifi_handler_t* handler);


/*! \brief Sending data packages to the connected TCP client
 *  \param handler Pointer to wifi_handler_t struct
 *  \param data    Pointer to data array to be sent
 *  \param len     Length of data array
 *  \return        Number of written bytes
 */
size_t wifi_handler_send_data(wifi_handler_t* handler, const uint8_t *data, size_t len);


/*! \brief Sending a null-terminated string to the connected TCP client
 *  \param handler Pointer to wifi_handler_t struct
 *  \param msg     Null-terminated string to be sent
 */
void wifi_handler_broadcast_string(wifi_handler_t* handler, const char *msg);


/*! \brief Sending data packages to the connected TCP client
 *  \param handler  Pointer to wifi_handler_t struct
 *  \param data     Pointer to data array to be sent
 *  \param data_len Length of data array
 */
void wifi_handler_broadcast_data(wifi_handler_t* handler, uint8_t *data, uint8_t data_len);

#endif
