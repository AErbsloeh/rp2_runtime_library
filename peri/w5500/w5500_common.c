#include "peri/w5500/w5500_common.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/time.h"
#include "pico/platform.h"


#define W5500_PIN_NOT_WIRED 0xFFu


#define REG_MR          0x0000
#define REG_GAR         0x0001
#define REG_SUBR        0x0005
#define REG_SHAR        0x0009
#define REG_SIPR        0x000F
#define REG_PHYCFGR     0x002E
#define REG_VERSIONR    0x0039
#define MR_RST          0x80

#define SN_SR           0x0003
#define SN_CR           0x0001
#define SN_RXBUF_SIZE   0x001E
#define SN_TXBUF_SIZE   0x001F

#define BSB_COMMON      0x00

static inline void cs_select(w5500_t *dev)
{
    gpio_put(dev->gpio_cs, 0);
}

static inline void cs_deselect(w5500_t *dev)
{
    gpio_put(dev->gpio_cs, 1);
}

static void send_header(w5500_t *dev, uint16_t addr, uint8_t block, bool write)
{
    uint8_t header[3];
    header[0] = (uint8_t)(addr >> 8);
    header[1] = (uint8_t)(addr & 0xFF);
    header[2] = (uint8_t)((block << 3) | (write ? 0x04 : 0x00));
    spi_write_blocking(dev->spi->spi_mod, header, 3);
}

void w5500_priv_read_buf(w5500_t *dev, uint16_t addr, uint8_t block,
                          uint8_t *buf, size_t len)
{
    cs_select(dev);
    send_header(dev, addr, block, false);
    spi_read_blocking(dev->spi->spi_mod, 0x00, buf, len);
    cs_deselect(dev);
}

void w5500_priv_write_buf(w5500_t *dev, uint16_t addr, uint8_t block,
                           const uint8_t *buf, size_t len)
{
    cs_select(dev);
    send_header(dev, addr, block, true);
    spi_write_blocking(dev->spi->spi_mod, buf, len);
    cs_deselect(dev);
}

static uint8_t reg_read8(w5500_t *dev, uint16_t addr, uint8_t block)
{
    uint8_t val;
    w5500_priv_read_buf(dev, addr, block, &val, 1);
    return val;
}

static void reg_write8(w5500_t *dev, uint16_t addr, uint8_t block, uint8_t val)
{
    w5500_priv_write_buf(dev, addr, block, &val, 1);
}

static uint16_t reg_read16(w5500_t *dev, uint16_t addr, uint8_t block)
{
    uint8_t buf[2];
    w5500_priv_read_buf(dev, addr, block, buf, 2);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

static void reg_write16(w5500_t *dev, uint16_t addr, uint8_t block, uint16_t val)
{
    uint8_t buf[2] = { (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    w5500_priv_write_buf(dev, addr, block, buf, 2);
}

static uint8_t sock_read8(w5500_t *dev, uint8_t sock, uint16_t off)
{
    return reg_read8(dev, off, BSB_SOCKET_REG(sock));
}

void w5500_priv_sock_write8(w5500_t *dev, uint8_t sock, uint16_t off, uint8_t val)
{
    reg_write8(dev, off, BSB_SOCKET_REG(sock), val);
}

uint16_t w5500_priv_sock_read16(w5500_t *dev, uint8_t sock, uint16_t off)
{
    return reg_read16(dev, off, BSB_SOCKET_REG(sock));
}

void w5500_priv_sock_write16(w5500_t *dev, uint8_t sock, uint16_t off, uint16_t val)
{
    reg_write16(dev, off, BSB_SOCKET_REG(sock), val);
}

void w5500_priv_sock_exec_cmd(w5500_t *dev, uint8_t sock, uint8_t cmd)
{
    w5500_priv_sock_write8(dev, sock, SN_CR, cmd);
    while (sock_read8(dev, sock, SN_CR) != 0x00) {
        tight_loop_contents();
    }
}

static void hw_reset(w5500_t *dev)
{
    gpio_put(dev->gpio_rstn, 0);
    sleep_ms(2);
    gpio_put(dev->gpio_rstn, 1);
    sleep_ms(5);
}

static void sw_reset(w5500_t *dev)
{
    reg_write8(dev, REG_MR, BSB_COMMON, MR_RST);
    sleep_ms(5);
}

void w5500_reset(w5500_t *dev)
{
    if (dev->gpio_rstn != W5500_PIN_NOT_WIRED) {
        hw_reset(dev);
    } else {
        sw_reset(dev);
    }
}

bool w5500_init(w5500_t *dev)
{
    gpio_init(dev->gpio_cs);
    gpio_pull_up(dev->gpio_cs);
    gpio_set_dir(dev->gpio_cs, GPIO_OUT);
    cs_deselect(dev);

    if (dev->gpio_rstn != W5500_PIN_NOT_WIRED) {
        gpio_init(dev->gpio_rstn);
        gpio_pull_up(dev->gpio_rstn);
        gpio_set_dir(dev->gpio_rstn, GPIO_OUT);
        gpio_put(dev->gpio_rstn, true);
    }

    if (!dev->spi->init_done) {
        configure_spi_module(dev->spi, false);
    }
    if (!dev->spi->init_done) {
        return false;
    }

    w5500_reset(dev);

    for (uint8_t s = 0; s < W5500_MAX_SOCKETS; s++) {
        w5500_priv_sock_write8(dev, s, SN_RXBUF_SIZE, 2);
        w5500_priv_sock_write8(dev, s, SN_TXBUF_SIZE, 2);
    }

    dev->init_done = true;
    return true;
}

uint8_t w5500_get_version(w5500_t *dev)
{
    return reg_read8(dev, REG_VERSIONR, BSB_COMMON);
}

bool w5500_get_link_up(w5500_t *dev)
{
    uint8_t phy = reg_read8(dev, REG_PHYCFGR, BSB_COMMON);
    return (phy & 0x01) != 0;
}

void w5500_set_mac(w5500_t *dev, const uint8_t mac[6])
{
    w5500_priv_write_buf(dev, REG_SHAR, BSB_COMMON, mac, 6);
}

void w5500_get_mac(w5500_t *dev, uint8_t mac_out[6])
{
    w5500_priv_read_buf(dev, REG_SHAR, BSB_COMMON, mac_out, 6);
}

void w5500_set_ip(w5500_t *dev, const uint8_t ip[4])
{
    w5500_priv_write_buf(dev, REG_SIPR, BSB_COMMON, ip, 4);
}

void w5500_get_ip(w5500_t *dev, uint8_t ip_out[4])
{
    w5500_priv_read_buf(dev, REG_SIPR, BSB_COMMON, ip_out, 4);
}

void w5500_set_gateway(w5500_t *dev, const uint8_t gw[4])
{
    w5500_priv_write_buf(dev, REG_GAR, BSB_COMMON, gw, 4);
}

void w5500_get_gateway(w5500_t *dev, uint8_t gw_out[4])
{
    w5500_priv_read_buf(dev, REG_GAR, BSB_COMMON, gw_out, 4);
}

void w5500_set_subnet(w5500_t *dev, const uint8_t mask[4])
{
    w5500_priv_write_buf(dev, REG_SUBR, BSB_COMMON, mask, 4);
}

void w5500_get_subnet(w5500_t *dev, uint8_t mask_out[4])
{
    w5500_priv_read_buf(dev, REG_SUBR, BSB_COMMON, mask_out, 4);
}

w5500_sock_status_t w5500_socket_status(w5500_t *dev, uint8_t sock)
{
    return (w5500_sock_status_t)sock_read8(dev, sock, SN_SR);
}