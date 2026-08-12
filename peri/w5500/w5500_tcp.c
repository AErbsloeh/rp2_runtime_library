#include "peri/w5500/w5500_tcp.h"

#define SN_MR           0x0000
#define SN_PORT         0x0004
#define SN_DIPR         0x000C
#define SN_DPORT        0x0010
#define SN_IR           0x0002
#define SN_TX_FSR       0x0020
#define SN_TX_WR        0x0024
#define SN_RX_RSR       0x0026
#define SN_RX_RD        0x0028

#define SN_MR_TCP       0x01

#define SN_CR_OPEN      0x01
#define SN_CR_LISTEN    0x02
#define SN_CR_CONNECT   0x04
#define SN_CR_DISCON    0x08
#define SN_CR_CLOSE     0x10
#define SN_CR_SEND      0x20
#define SN_CR_RECV      0x40

#define BSB_SOCKET_TXBUF(n)   (0x02 + ((n) * 4))
#define BSB_SOCKET_RXBUF(n)   (0x03 + ((n) * 4))

bool w5500_tcp_listen(w5500_tcp_t *conn)
{
    if (conn->sock >= W5500_MAX_SOCKETS) return false;

    w5500_priv_sock_write8(conn->dev, conn->sock, SN_MR, SN_MR_TCP);
    w5500_priv_sock_write16(conn->dev, conn->sock, SN_PORT, conn->src_port);
    w5500_priv_sock_exec_cmd(conn->dev, conn->sock, SN_CR_OPEN);

    if (w5500_tcp_status(conn) != W5500_SOCK_INIT) {
        return false;
    }

    w5500_priv_sock_exec_cmd(conn->dev, conn->sock, SN_CR_LISTEN);
    return w5500_tcp_status(conn) == W5500_SOCK_LISTEN;
}

bool w5500_tcp_connect(w5500_tcp_t *conn)
{
    if (conn->sock >= W5500_MAX_SOCKETS) return false;

    w5500_priv_sock_write8(conn->dev, conn->sock, SN_MR, SN_MR_TCP);
    w5500_priv_sock_write16(conn->dev, conn->sock, SN_PORT, conn->src_port);
    w5500_priv_sock_exec_cmd(conn->dev, conn->sock, SN_CR_OPEN);

    if (w5500_tcp_status(conn) != W5500_SOCK_INIT) {
        return false;
    }

    w5500_priv_write_buf(conn->dev, SN_DIPR, BSB_SOCKET_REG(conn->sock),
                          conn->dest_ip, 4);
    w5500_priv_sock_write16(conn->dev, conn->sock, SN_DPORT, conn->dest_port);

    w5500_priv_sock_exec_cmd(conn->dev, conn->sock, SN_CR_CONNECT);
    return true;
}

void w5500_tcp_close(w5500_tcp_t *conn)
{
    if (conn->sock >= W5500_MAX_SOCKETS) return;

    w5500_sock_status_t status = w5500_tcp_status(conn);
    if (status == W5500_SOCK_ESTABLISHED || status == W5500_SOCK_CLOSE_WAIT) {
        w5500_priv_sock_exec_cmd(conn->dev, conn->sock, SN_CR_DISCON);
    }
    w5500_priv_sock_exec_cmd(conn->dev, conn->sock, SN_CR_CLOSE);
    w5500_priv_sock_write8(conn->dev, conn->sock, SN_IR, 0xFF);
}

w5500_sock_status_t w5500_tcp_status(w5500_tcp_t *conn)
{
    return w5500_socket_status(conn->dev, conn->sock);
}

int w5500_tcp_send(w5500_tcp_t *conn, const uint8_t *data, uint16_t len)
{
    if (conn->sock >= W5500_MAX_SOCKETS) return -1;
    if (w5500_tcp_status(conn) != W5500_SOCK_ESTABLISHED) return -1;
    if (len == 0) return 0;

    uint16_t free_size;
    do {
        free_size = w5500_priv_sock_read16(conn->dev, conn->sock, SN_TX_FSR);
        if (w5500_tcp_status(conn) != W5500_SOCK_ESTABLISHED) return -1;
    } while (free_size < len);

    uint16_t wr_ptr = w5500_priv_sock_read16(conn->dev, conn->sock, SN_TX_WR);

    uint16_t buf_size = 2048;
    uint16_t offset = wr_ptr % buf_size;
    uint16_t first_chunk = buf_size - offset;
    if (first_chunk > len) first_chunk = len;

    w5500_priv_write_buf(conn->dev, offset, BSB_SOCKET_TXBUF(conn->sock),
                          data, first_chunk);
    if (first_chunk < len) {
        w5500_priv_write_buf(conn->dev, 0, BSB_SOCKET_TXBUF(conn->sock),
                              data + first_chunk, len - first_chunk);
    }

    w5500_priv_sock_write16(conn->dev, conn->sock, SN_TX_WR, (uint16_t)(wr_ptr + len));
    w5500_priv_sock_exec_cmd(conn->dev, conn->sock, SN_CR_SEND);

    return (int)len;
}

uint16_t w5500_tcp_available(w5500_tcp_t *conn)
{
    if (conn->sock >= W5500_MAX_SOCKETS) return 0;
    return w5500_priv_sock_read16(conn->dev, conn->sock, SN_RX_RSR);
}

int w5500_tcp_recv(w5500_tcp_t *conn, uint8_t *buf, uint16_t maxlen)
{
    if (conn->sock >= W5500_MAX_SOCKETS) return -1;

    w5500_sock_status_t status = w5500_tcp_status(conn);
    if (status == W5500_SOCK_CLOSED) {
        return -1;
    }

    uint16_t avail = w5500_tcp_available(conn);
    if (avail == 0) {
        if (status == W5500_SOCK_CLOSE_WAIT) {
            return -1;
        }
        return 0;
    }

    uint16_t to_read = (avail < maxlen) ? avail : maxlen;
    uint16_t rd_ptr = w5500_priv_sock_read16(conn->dev, conn->sock, SN_RX_RD);

    uint16_t buf_size = 2048;
    uint16_t offset = rd_ptr % buf_size;
    uint16_t first_chunk = buf_size - offset;
    if (first_chunk > to_read) first_chunk = to_read;

    w5500_priv_read_buf(conn->dev, offset, BSB_SOCKET_RXBUF(conn->sock),
                         buf, first_chunk);
    if (first_chunk < to_read) {
        w5500_priv_read_buf(conn->dev, 0, BSB_SOCKET_RXBUF(conn->sock),
                             buf + first_chunk, to_read - first_chunk);
    }

    w5500_priv_sock_write16(conn->dev, conn->sock, SN_RX_RD, (uint16_t)(rd_ptr + to_read));
    w5500_priv_sock_exec_cmd(conn->dev, conn->sock, SN_CR_RECV);

    return (int)to_read;
}