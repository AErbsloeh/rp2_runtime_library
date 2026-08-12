#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "peri/w5500/w5500_udp.h"


#define LOCAL_UDP_PORT   30000   /* port the Pico listens on */


int main(void)
{
    static spi_rp2_t w5500_spi_inst = {
        .spi_mod = spi0,
        .pin_mosi = 19,
        .pin_sclk = 18,
        .pin_miso = 16,
        .fspi_khz = 8000,
        .mode = 0,
        .msb_first = true,
        .init_done = false
    };
    static w5500_udp_t w5500_config = {
        .hw = {
            .spi = &w5500_spi_inst,
            .gpio_cs = 17,
            .gpio_rstn = 20,
            .gpio_intn = 21,
            .init_done = false,
        },
        .buffer_size = 2048,
        .sock = 0,
        .multicast_ip = {224, 0, 0, 5},
        .multicast_port = 30000,
        .dest_ip = {192, 168, 11, 3},
        .dest_port = 30001,
    };

    stdio_init_all();
    sleep_ms(2000);

    if (!w5500_udp_init(&w5500_config)) {
        printf("W5500 init failed!\n");
        while (true) tight_loop_contents();
    }

    printf("Waiting for link...\n");
    w5500_udp_wait_until_connected(&w5500_config);
    printf("Link is up.\n");
    w5500_udp_print_info(&w5500_config);

    if (!w5500_udp_open(&w5500_config, LOCAL_UDP_PORT)) {
        printf("Could not open UDP socket!\n");
        while (true) tight_loop_contents();
    }
    printf("UDP socket opened on port %d.\n", LOCAL_UDP_PORT);

    /* w5500_config.dest_ip/dest_port already point at the PC (see the
     * static initializer in w5500_udp.h) - adjust there if needed. */

    uint8_t rx_buf[256];

    while (true) {
        int32_t n = w5500_udp_recv(&w5500_config, rx_buf, sizeof(rx_buf) - 1);
        if (n > 0) {
            rx_buf[n] = '\0';
            printf("Received from %d.%d.%d.%d:%d -> %s\n",
                   w5500_config.last_src_ip[0], w5500_config.last_src_ip[1],
                   w5500_config.last_src_ip[2], w5500_config.last_src_ip[3],
                   w5500_config.last_src_port, rx_buf);

            /* Echo it straight back to the last sender */
            memcpy(w5500_config.dest_ip, w5500_config.last_src_ip, 4);
            w5500_config.dest_port = w5500_config.last_src_port;

            const char *reply = "ACK\n";
            w5500_udp_send(&w5500_config, (const uint8_t *)reply, (uint16_t)strlen(reply));
        }

        sleep_ms(10);
    }
}