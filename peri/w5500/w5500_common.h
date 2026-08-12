#ifndef W5500_COMMON_H
#define W5500_COMMON_H


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal/spi/spi.h"


/*! \brief Shared hardware/device configuration for the W5500.
 * \param spi        Pointer to an SPI HAL instance (spi_rp2_t), already populated with pins/frequency.
 * \param gpio_cs     Chip-select pin. MANDATORY - always driven by the MCU for every SPI transfer.
 * \param gpio_rstn   Hardware reset pin, or 0xFF if not wired up.
 *                    OPTIONAL - if 0xFF, a software reset via the MR register is used instead.
 * \param gpio_intn   INTn pin, or 0xFF if not used. OPTIONAL - never read by this driver (polling-based); provided only as storage for callers implementing their own IRQ handling.
 * \param init_done   Set to true once initialization has completed successfully.
 */
typedef struct {
    spi_rp2_t *spi;
    uint8_t gpio_cs;
    uint8_t gpio_rstn;
    uint8_t gpio_intn;
    bool init_done;
} w5500_t;

/*! \brief Number of hardware sockets available on the W5500. */
#define W5500_MAX_SOCKETS 8

/*! \brief Socket status values (Sn_SR register). Applies to any socket
 * regardless of the protocol (TCP/UDP/MACRAW) it is configured for.
 */
typedef enum {
    W5500_SOCK_CLOSED      = 0x00, /*!< Socket is closed / unused */
    W5500_SOCK_INIT        = 0x13, /*!< Opened, ready for listen/connect */
    W5500_SOCK_LISTEN      = 0x14, /*!< TCP server, waiting for a client */
    W5500_SOCK_SYNSENT     = 0x15, /*!< TCP client, SYN sent */
    W5500_SOCK_SYNRECV     = 0x16, /*!< TCP server, SYN received */
    W5500_SOCK_ESTABLISHED = 0x17, /*!< TCP connection established */
    W5500_SOCK_FIN_WAIT    = 0x18, /*!< TCP connection closing */
    W5500_SOCK_CLOSING     = 0x1A, /*!< TCP connection closing */
    W5500_SOCK_TIME_WAIT   = 0x1B, /*!< TCP connection closing */
    W5500_SOCK_CLOSE_WAIT  = 0x1C, /*!< Peer has closed, local side may still send/must close */
    W5500_SOCK_LAST_ACK    = 0x1D, /*!< TCP connection closing */
    W5500_SOCK_UDP         = 0x22, /*!< Socket opened in UDP mode */
    W5500_SOCK_MACRAW      = 0x42, /*!< Socket opened in MACRAW mode */
} w5500_sock_status_t;

/*! \brief Initializes the W5500 using the spi/gpio_cs/gpio_rstn/gpio_intn
 * already set in *dev: configures the relevant GPIOs, brings up the SPI
 * HAL if needed, resets the chip, and assigns default TX/RX buffer sizes
 * to all W5500_MAX_SOCKETS sockets.
 *
 * \param dev  Device handle, pre-populated with spi/gpio_cs/gpio_rstn/gpio_intn.
 * \return     true on success, false if the SPI HAL failed to initialize.
 */
bool w5500_init(w5500_t *dev);

/*! \brief Resets the chip. Performs a hardware reset via gpio_rstn if
 * that pin is wired up (!= 0xFF), otherwise falls back to a software
 * reset via the MR register.
 *
 * \param dev  Device handle.
 */
void w5500_reset(w5500_t *dev);

/*! \brief Reads the VERSIONR register.
 * \param dev  Device handle.
 * \return     Chip version, always 0x04 for the W5500.
 */
uint8_t w5500_get_version(w5500_t *dev);

/*! \brief Reads the PHY link state.
 * \param dev  Device handle.
 * \return     true if a physical Ethernet link is currently detected.
 */
bool w5500_get_link_up(w5500_t *dev);

/*! \brief Sets the MAC address.
 * \param dev  Device handle.
 * \param mac  6-byte MAC address.
 */
void w5500_set_mac(w5500_t *dev, const uint8_t mac[6]);

/*! \brief Reads the currently configured MAC address.
 * \param dev      Device handle.
 * \param mac_out  Destination buffer, at least 6 bytes.
 */
void w5500_get_mac(w5500_t *dev, uint8_t mac_out[6]);

/*! \brief Sets the local IP address.
 * \param dev  Device handle.
 * \param ip   4-byte IPv4 address.
 */
void w5500_set_ip(w5500_t *dev, const uint8_t ip[4]);

/*! \brief Reads the currently configured local IP address.
 * \param dev     Device handle.
 * \param ip_out  Destination buffer, at least 4 bytes.
 */
void w5500_get_ip(w5500_t *dev, uint8_t ip_out[4]);

/*! \brief Sets the gateway address.
 * \param dev  Device handle.
 * \param gw   4-byte IPv4 address.
 */
void w5500_set_gateway(w5500_t *dev, const uint8_t gw[4]);

/*! \brief Reads the currently configured gateway address.
 * \param dev     Device handle.
 * \param gw_out  Destination buffer, at least 4 bytes.
 */
void w5500_get_gateway(w5500_t *dev, uint8_t gw_out[4]);

/*! \brief Sets the subnet mask.
 * \param dev   Device handle.
 * \param mask  4-byte subnet mask.
 */
void w5500_set_subnet(w5500_t *dev, const uint8_t mask[4]);

/*! \brief Reads the currently configured subnet mask.
 * \param dev       Device handle.
 * \param mask_out  Destination buffer, at least 4 bytes.
 */
void w5500_get_subnet(w5500_t *dev, uint8_t mask_out[4]);

/*! \brief Reads the current status of a socket.
 * \param dev   Device handle.
 * \param sock  Hardware socket number (0..W5500_MAX_SOCKETS-1).
 * \return      Current Sn_SR value as w5500_sock_status_t.
 */
w5500_sock_status_t w5500_socket_status(w5500_t *dev, uint8_t sock);

/*! \brief Block-select offset for a socket's register block.
 * Internal - shared between w5500_common.c and w5500_tcp.c.
 */
#define BSB_SOCKET_REG(n) (0x01 + ((n) * 4))

/*! \brief Reads len bytes starting at addr within the given SPI block.
 * Internal - implemented in w5500_common.c, used by w5500_tcp.c.
 */
void w5500_priv_read_buf(w5500_t *dev, uint16_t addr, uint8_t block,
                          uint8_t *buf, size_t len);

/*! \brief Writes len bytes starting at addr within the given SPI block.
 * Internal - implemented in w5500_common.c, used by w5500_tcp.c.
 */
void w5500_priv_write_buf(w5500_t *dev, uint16_t addr, uint8_t block,
                           const uint8_t *buf, size_t len);

/*! \brief Reads a 16-bit socket register.
 * Internal - implemented in w5500_common.c, used by w5500_tcp.c.
 */
uint16_t w5500_priv_sock_read16(w5500_t *dev, uint8_t sock, uint16_t off);

/*! \brief Writes a 16-bit socket register.
 * Internal - implemented in w5500_common.c, used by w5500_tcp.c.
 */
void w5500_priv_sock_write16(w5500_t *dev, uint8_t sock, uint16_t off, uint16_t val);

/*! \brief Writes an 8-bit socket register.
 * Internal - implemented in w5500_common.c, used by w5500_tcp.c.
 */
void w5500_priv_sock_write8(w5500_t *dev, uint8_t sock, uint16_t off, uint8_t val);

/*! \brief Writes cmd to Sn_CR and blocks until the chip has processed it.
 * Internal - implemented in w5500_common.c, used by w5500_tcp.c.
 */
void w5500_priv_sock_exec_cmd(w5500_t *dev, uint8_t sock, uint8_t cmd);


#endif