/**
 * example_main_tcp.c
 *
 * Example: initializes the W5500 on the WIZ820io module, prints the
 * chip version and link status, then connects as a TCP client to a
 * server, sends a message and prints the response.
 *
 * Adjust the pin assignment to your wiring. Example for SPI0 on the Pico:
 *   SCK  -> GPIO 18
 *   MOSI -> GPIO 19
 *   MISO -> GPIO 16
 *   CS   -> GPIO 17
 *   RST  -> GPIO 20 (optional - set gpio_rstn to 0xFF below if unused)
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "peri/w5500/w5500_common.h"
#include "peri/w5500/w5500_tcp.h"

static spi_rp2_t w5500_spi_inst = {
    .spi_mod = spi0,
    .pin_mosi = 19,
    .pin_sclk = 18,
    .pin_miso = 16,
    .fspi_khz = 20000, /* 20 MHz */
    .mode = 0,
    .msb_first = true,
    .init_done = false
};

static w5500_t w5500_dev = {
    .spi = &w5500_spi_inst,
    .gpio_cs = 17,
    .gpio_rstn = 20,   /* set to 0xFF if no reset pin is wired up */
    .gpio_intn = 0xFF, /* not used by this driver (polling-based) */
    .init_done = false,
};

static w5500_tcp_t tcp_conn = {
    .dev = &w5500_dev,
    .sock = 0,
    .dest_ip = { 192, 168, 1, 100 },
    .dest_port = 5000,
    .src_port = 50000,
};

int main(void)
{
    stdio_init_all();
    sleep_ms(2000); /* time for the USB serial connection to come up */

    if (!w5500_init(&w5500_dev)) {
        printf("W5500 init failed!\n");
        while (true) tight_loop_contents();
    }

    printf("W5500 chip version: 0x%02X (expected 0x04)\n", w5500_get_version(&w5500_dev));

    /* Set network configuration */
    uint8_t mac[6]     = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
    uint8_t ip[4]       = { 192, 168, 1, 50 };
    uint8_t gateway[4]  = { 192, 168, 1, 1 };
    uint8_t subnet[4]   = { 255, 255, 255, 0 };

    w5500_set_mac(&w5500_dev, mac);
    w5500_set_ip(&w5500_dev, ip);
    w5500_set_gateway(&w5500_dev, gateway);
    w5500_set_subnet(&w5500_dev, subnet);

    printf("Waiting for Ethernet link...\n");
    while (!w5500_get_link_up(&w5500_dev)) {
        sleep_ms(200);
    }
    printf("Link is up.\n");

    printf("Connecting to %d.%d.%d.%d:%d ...\n",
           tcp_conn.dest_ip[0], tcp_conn.dest_ip[1],
           tcp_conn.dest_ip[2], tcp_conn.dest_ip[3], tcp_conn.dest_port);

    w5500_tcp_connect(&tcp_conn);

    /* Wait for the connection to be established */
    w5500_sock_status_t status;
    do {
        status = w5500_tcp_status(&tcp_conn);
        sleep_ms(50);
    } while (status != W5500_SOCK_ESTABLISHED && status != W5500_SOCK_CLOSED);

    if (status != W5500_SOCK_ESTABLISHED) {
        printf("Connection failed.\n");
        return 1;
    }
    printf("Connected!\n");

    /* Send a message */
    const char *msg = "Hello from the Pico!\n";
    w5500_tcp_send(&tcp_conn, (const uint8_t *)msg, (uint16_t)strlen(msg));

    /* Wait for a response and print it */
    uint8_t rx_buf[256];
    while (true) {
        int n = w5500_tcp_recv(&tcp_conn, rx_buf, sizeof(rx_buf) - 1);
        if (n > 0) {
            rx_buf[n] = '\0';
            printf("Received: %s\n", rx_buf);
        } else if (n < 0) {
            printf("Connection closed.\n");
            break;
        }
        sleep_ms(50);
    }

    w5500_tcp_close(&tcp_conn);

    while (true) {
        tight_loop_contents();
    }
}