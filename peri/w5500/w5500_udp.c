#include "peri/w5500/w5500_udp.h"
#include "wizchip_conf.h"
#include "wizchip_spi.h"
#include "socket.h"
#include "multicast.h"
#include <stdio.h>


/* Never a valid RP2040 GPIO number - used as the "pin not wired up" sentinel */
#define W5500_PIN_NOT_WIRED 0xFFu


static uint8_t wizchip_read(void)
{
    uint8_t rx_data = 0x00;
    uint8_t tx_data = 0xFF;
    spi_read_blocking(spi0, tx_data, &rx_data, 1);
    return rx_data;
}

static void wizchip_write(uint8_t tx_data)
{
    spi_write_blocking(spi0, &tx_data, 1);
}

/* Note: the ioLibrary callbacks don't take a context parameter, so
 * only ONE W5500 module can be driven at a time with this library.
 * This pin must match w5500_config.hw.gpio_cs. */
static inline void wizchip_select(void)
{
    gpio_put(w5500_config.hw.gpio_cs, false);
}

static inline void wizchip_deselect(void)
{
    gpio_put(w5500_config.hw.gpio_cs, true);
}

static uint8_t ethernet_buf[2048] = {
    0,
};

static wiz_NetInfo g_net_info = {
    .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56},    // MAC address
    .ip = {192, 168, 11, 2},                        // IP address
    .sn = {255, 255, 255, 0},                       // Subnet Mask
    .gw = {192, 168, 11, 1},                        // Gateway
    .dns = {8, 8, 8, 8},                            // DNS Server
    .dhcp = NETINFO_STATIC                          // DHCP enable/disable
};

bool w5500_udp_init(w5500_udp_t *config)
{
    /* gpio_cs is mandatory: always configure and drive it */
    gpio_init(config->hw.gpio_cs);
    gpio_pull_up(config->hw.gpio_cs);
    gpio_set_dir(config->hw.gpio_cs, GPIO_OUT);
    gpio_put(config->hw.gpio_cs, true);

    /* gpio_rstn is optional - only touch the pin if it's actually wired up.
     * If not wired up, w5500_udp_do_reset() falls back to a software reset. */
    if (config->hw.gpio_rstn != W5500_PIN_NOT_WIRED) {
        gpio_init(config->hw.gpio_rstn);
        gpio_pull_up(config->hw.gpio_rstn);
        gpio_set_dir(config->hw.gpio_rstn, GPIO_OUT);
        gpio_put(config->hw.gpio_rstn, false);
    }

    /* gpio_intn is optional and not read anywhere in this driver (polling-
     * based); only configured here if the caller actually wired it up. */
    if (config->hw.gpio_intn != W5500_PIN_NOT_WIRED) {
        gpio_init(config->hw.gpio_intn);
        gpio_pull_up(config->hw.gpio_intn);
        gpio_set_dir(config->hw.gpio_intn, GPIO_IN);
    }
    sleep_ms(1);

    if (!config->hw.spi->init_done) {
        configure_spi_module(config->hw.spi, false);
    }

    wizchip_cris_initialize();
    w5500_udp_do_reset(config);

    // --- INIT PHASE
    /* CS function register */
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
    /* SPI function register */
    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);

    /* W5x00, W6x00 initialize */
    uint8_t memsize[2][8] = {{2, 2, 2, 2, 2, 2, 2, 2}, {2, 2, 2, 2, 2, 2, 2, 2}};
    if (ctlwizchip(CW_INIT_WIZCHIP, (void *)memsize) == -1) {
        config->hw.init_done = false;
        return false;
    } else {
        network_initialize(g_net_info);
        config->hw.init_done = true;
    }
    return config->hw.init_done;
}

void w5500_udp_do_reset(w5500_udp_t *config)
{
    if (config->hw.gpio_rstn != W5500_PIN_NOT_WIRED) {
        /* hardware reset via the RST pin */
        gpio_put(config->hw.gpio_rstn, false);
        sleep_ms(100);
        gpio_put(config->hw.gpio_rstn, true);
        sleep_ms(100);
    } else {
        /* NOTE: assumes the ioLibrary's setMR()/MR_RST are available
         * (wizchip_conf.h / w5500's register conf header) to perform a
         * software reset when no RST pin is wired up. If your ioLibrary
         * version doesn't expose setMR(), let me know the header and
         * I'll adjust this to whatever it does provide. */
        setMR(MR_RST);
        sleep_ms(5); /* the chip clears the RST bit by itself */
    }
    config->hw.init_done = false;
}

bool w5500_udp_phy_connected(w5500_udp_t *config)
{
    uint8_t temp;
    if (config->hw.init_done) {
        ctlwizchip(CW_GET_PHYLINK, &temp);
        return temp == PHY_LINK_ON;
    }
    return false;
}

void w5500_udp_wait_until_connected(w5500_udp_t *config)
{
    uint8_t temp;
    if (config->hw.init_done)
        do {
            sleep_ms(100);
            if (ctlwizchip(CW_GET_PHYLINK, (void *)&temp) == -1) {
                return;
            }
        } while (temp == PHY_LINK_OFF);
}

void w5500_udp_print_info(w5500_udp_t *config)
{
    (void)config;
    print_network_information(g_net_info);
}

void w5500_udp_test(w5500_udp_t *config)
{
    if (config->hw.init_done) {
        /* NOTE: state machine - must be called repeatedly (e.g. in the
         * main loop). The first call only opens the socket and joins
         * the multicast group, data is only received afterwards. */
        multicast_recv(config->sock, ethernet_buf,
                        config->multicast_ip, config->multicast_port);
    }
}

/* ------------------------------------------------------------------- */
/* Simple unicast UDP functions                                         */
/* ------------------------------------------------------------------- */

bool w5500_udp_open(w5500_udp_t *config, uint16_t local_port)
{
    if (!config->hw.init_done) return false;

    close(config->sock); /* make sure the socket is free beforehand */

    if (socket(config->sock, Sn_MR_UDP, local_port, 0) != config->sock) {
        return false;
    }
    return true;
}

int32_t w5500_udp_send(w5500_udp_t *config, const uint8_t *data, uint16_t len)
{
    if (!config->hw.init_done) return -1;
    return sendto(config->sock, (uint8_t *)data, len,
                   config->dest_ip, config->dest_port);
}

int32_t w5500_udp_recv(w5500_udp_t *config, uint8_t *buf, uint16_t maxlen)
{
    if (!config->hw.init_done) return -1;

    /* the ioLibrary's recvfrom() is already non-blocking and returns
     * 0 if nothing is available. */
    int32_t n = recvfrom(config->sock, buf, maxlen,
                          config->last_src_ip, &config->last_src_port);
    return n;
}

void w5500_udp_close(w5500_udp_t *config)
{
    close(config->sock);
}