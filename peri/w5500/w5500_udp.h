#ifndef W5500_UDP_H_
#define W5500_UDP_H_


#include "peri/w5500/w5500_common.h"


/*! \brief UDP-specific configuration, on top of the shared w5500_t hardware config (see w5500_common.h) - reused as-is here since it only holds pin/SPI configuration, nothing TCP- or ioLibrary-specific.
 *
 * \param hw              Shared hardware/device configuration
 * \param buffer_size     Buffer size for Ethernet communication
 * \param sock            Hardware socket number (0..7) used by the
 *                        unicast functions (w5500_udp_open/send/recv/close)
 * \param multicast_ip    Multicast group address used by w5500_udp_test()
 * \param multicast_port  Multicast group port used by w5500_udp_test()
 * \param dest_ip         Unicast destination address, used by w5500_udp_send()
 * \param dest_port       Unicast destination port, used by w5500_udp_send()
 * \param last_src_ip     Filled in by w5500_udp_recv() with the sender's address
 * \param last_src_port   Filled in by w5500_udp_recv() with the sender's port
 */
typedef struct {
    w5500_t *hw;
    uint16_t buffer_size;
    uint8_t sock;
    uint8_t multicast_ip[4];
    uint16_t multicast_port;
    uint8_t dest_ip[4];
    uint16_t dest_port;
    uint8_t last_src_ip[4];
    uint16_t last_src_port;
} w5500_udp_t;


/*! \brief Resets the W5500 Ethernet module. Uses a hardware reset via
 * gpio_rstn if that pin is wired up (!= 0xFF), otherwise falls back to
 * a software reset via the W5500's MR register automatically.
 *
 * \param config  Pointer to the W5500 Ethernet module configuration struct.
 */
void w5500_udp_do_reset(w5500_udp_t *config);


/*! \brief Initializes the W5500 Ethernet module using the SPI interface.
 *
 * \param config  Pointer to the W5500 Ethernet module configuration struct.
 * \return        true if initialization succeeded, false otherwise.
 */
bool w5500_udp_init(w5500_udp_t *config);


/*! \brief Checks the PHY link status.
 *
 * \param config  Pointer to the W5500 Ethernet module configuration struct.
 * \return        true if a physical link is up. Also returns false if
 *                config->hw.init_done is false (not just "no link").
 */
bool w5500_udp_phy_connected(w5500_udp_t *config);


/*! \brief Blocks until the PHY link comes up, polling every 100 ms.
 *
 * \param config  Pointer to the W5500 Ethernet module configuration struct.
 */
void w5500_udp_wait_until_connected(w5500_udp_t *config);


/*! \brief Prints the current network information (IP, subnet, gateway,
 * MAC, DNS) of the W5500 Ethernet module.
 *
 * \param config  Pointer to the W5500 Ethernet module configuration struct.
 */
void w5500_udp_print_info(w5500_udp_t *config);


/*! \brief Tests the multicast UDP connection of the W5500 Ethernet module.
 *
 * NOTE: this is a state machine (multicast_recv). Must be called
 * repeatedly (e.g. every loop iteration) - the first call only opens
 * the socket and joins the multicast group, data is only received on
 * subsequent calls.
 *
 * \param config  Pointer to the W5500 Ethernet module configuration struct.
 */
void w5500_udp_test(w5500_udp_t *config);


/*! \brief Opens a UDP socket (config->sock) on the given local port.
 *
 * \param config      Pointer to the W5500 Ethernet module configuration
 *                    struct; uses config->sock as the hardware socket
 *                    number (0..7).
 * \param local_port  Local UDP port to listen on.
 * \return            true on success.
 */
bool w5500_udp_open(w5500_udp_t *config, uint16_t local_port);


/*! \brief Sends data via UDP to config->dest_ip:config->dest_port, using
 * the socket stored in config->sock.
 *
 * \param config  Pointer to the W5500 Ethernet module configuration struct.
 * \param data    Data to send.
 * \param len     Number of bytes to send.
 * \return        Number of bytes sent, or <0 on error.
 */
int32_t w5500_udp_send(w5500_udp_t *config, const uint8_t *data, uint16_t len);


/*! \brief Reads received UDP data (non-blocking) from config->sock. On
 * success, config->last_src_ip and config->last_src_port are updated
 * with the sender's address/port.
 *
 * \param config  Pointer to the W5500 Ethernet module configuration struct.
 * \param buf     Destination buffer.
 * \param maxlen  Maximum number of bytes to read.
 * \return        Number of bytes read, 0 if nothing available, <0 on error.
 */
int32_t w5500_udp_recv(w5500_udp_t *config, uint8_t *buf, uint16_t maxlen);


/*! \brief Closes the socket stored in config->sock.
 *
 * \param config  Pointer to the W5500 Ethernet module configuration struct.
 */
void w5500_udp_close(w5500_udp_t *config);


#endif