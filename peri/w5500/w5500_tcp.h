#ifndef W5500_TCP_H
#define W5500_TCP_H


#include "peri/w5500/w5500_common.h"


/*! \brief One TCP connection (client or server) using the WIZnet
 * Ethernet Chip W5500.
 *
 * \param dev        Pointer to the shared hardware device handle (must
 *                   already be initialized via w5500_init()).
 * \param sock       Hardware socket number to use (0..7). Each
 *                   connection needs its own, unused socket.
 * \param dest_ip    Client:  who to connect to (set before
 *                   w5500_tcp_connect). Server:  filled in
 *                   automatically once a client connects, so you can
 *                   inspect who's talking to you after the fact.
 * \param dest_port  Client:  port to connect to (set before
 *                   w5500_tcp_connect). Server:  filled in
 *                   automatically once a client connects.
 * \param src_port   Client:  local source port (freely choosable, e.g.
 *                   50000 + sock). Server:  the port to listen on (set
 *                   before w5500_tcp_listen).
 */
typedef struct {
    w5500_t *dev;
    uint8_t sock;
    uint8_t dest_ip[4];
    uint16_t dest_port;
    uint16_t src_port;
} w5500_tcp_t;


/*! \brief Opens conn->sock as a TCP server on conn->src_port and puts
 * it into the LISTEN state. Afterwards, poll with w5500_tcp_status()
 * until it reaches W5500_SOCK_ESTABLISHED (= a client has connected);
 * dest_ip/dest_port are then filled in with the client's address.
 *
 * \param conn  TCP connection handle, with sock and src_port set.
 * \return      true if the socket reached the LISTEN state.
 */
bool w5500_tcp_listen(w5500_tcp_t *conn);


/*! \brief Opens conn->sock as a TCP client and connects to
 * conn->dest_ip:conn->dest_port from local port conn->src_port.
 * Non-blocking: poll w5500_tcp_status() afterwards until
 * W5500_SOCK_ESTABLISHED is reached.
 *
 * \param conn  TCP connection handle, with sock, dest_ip, dest_port
 *              and src_port set.
 * \return      true if the connect command was issued successfully.
 */
bool w5500_tcp_connect(w5500_tcp_t *conn);


/*! \brief Cleanly closes the connection (DISCON) and then the socket (CLOSE).
 *
 * \param conn  TCP connection handle.
 */
void w5500_tcp_close(w5500_tcp_t *conn);


/*! \brief Reads the current status of conn->sock.
 *
 * \param conn  TCP connection handle.
 * \return      Current socket status, see w5500_sock_status_t.
 */
w5500_sock_status_t w5500_tcp_status(w5500_tcp_t *conn);


/*! \brief Sends data over a connected TCP socket. Blocks until enough
 * free TX buffer space is available and the SEND command has completed.
 *
 * \param conn  TCP connection handle.
 * \param data  Data to send.
 * \param len   Number of bytes to send.
 * \return      Number of bytes sent, or -1 on error (not connected).
 */
int w5500_tcp_send(w5500_tcp_t *conn, const uint8_t *data, uint16_t len);


/*! \brief Reads received data, non-blocking.
 *
 * \param conn    TCP connection handle.
 * \param buf     Destination buffer.
 * \param maxlen  Maximum number of bytes to read.
 * \return        Number of bytes read, 0 if nothing available, or -1
 *                if the connection has been closed.
 */
int w5500_tcp_recv(w5500_tcp_t *conn, uint8_t *buf, uint16_t maxlen);


/*! \brief Number of bytes currently waiting in the receive buffer.
 *
 * \param conn  TCP connection handle.
 * \return      Number of bytes available to read.
 */
uint16_t w5500_tcp_available(w5500_tcp_t *conn);


#endif