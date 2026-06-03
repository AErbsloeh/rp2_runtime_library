#include "hal/transport/transport.h"
#include "hal/wifi/wifi_tcp.h"
#include "transport_wifi_config.h"
#include "pico/stdio_usb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TRANSPORT_WIFI_RX_QUEUE_SIZE
#define TRANSPORT_WIFI_RX_QUEUE_SIZE 256
#endif

static uint8_t rx_queue[TRANSPORT_WIFI_RX_QUEUE_SIZE];
static volatile size_t rx_head = 0;
static volatile size_t rx_tail = 0;

static wifi_handler_t wifi_handler = {
    .init_done = false,
    .use_dhcp = TRANSPORT_WIFI_USE_DHCP,
    .country = TRANSPORT_WIFI_COUNTRY,
    .port = TRANSPORT_WIFI_PORT,
    .hostname = TRANSPORT_WIFI_HOSTNAME,
    .ssid = TRANSPORT_WIFI_SSID,
    .password = TRANSPORT_WIFI_PASSWORD,
    .static_ip = TRANSPORT_WIFI_STATIC_IP,
    .netmask = TRANSPORT_WIFI_NETMASK,
    .gateway = TRANSPORT_WIFI_GATEWAY,
    .on_receive = NULL,
    .on_receive_context = NULL
};

static bool rx_queue_push(uint8_t byte)
{
    size_t next_head = (rx_head + 1) % TRANSPORT_WIFI_RX_QUEUE_SIZE;

    if (next_head == rx_tail) {
        return false;
    }

    rx_queue[rx_head] = byte;
    rx_head = next_head;
    return true;
}

static bool rx_queue_pop(uint8_t *byte)
{
    if (rx_tail == rx_head) {
        return false;
    }

    *byte = rx_queue[rx_tail];
    rx_tail = (rx_tail + 1) % TRANSPORT_WIFI_RX_QUEUE_SIZE;
    return true;
}

static void wifi_receive_callback(void *context, const uint8_t *data, size_t len)
{
    (void)context;

    for (size_t idx = 0; idx < len; idx++) {
        rx_queue_push(data[idx]);
    }
}

static void transport_push_rx_byte(transport_rx_buffer_t *rx_buffer, uint8_t byte)
{
    rx_buffer->data[rx_buffer->position] = (char)byte;

    if (rx_buffer->position == 0) {
        rx_buffer->position = rx_buffer->length - 1;
        rx_buffer->ready = true;
    } else {
        rx_buffer->position--;
        rx_buffer->ready = false;
    }
}

bool transport_init(transport_rx_buffer_t *rx_buffer)
{
    stdio_init_all();

    absolute_time_t usb_timeout = make_timeout_time_ms(8000);
    while (!stdio_usb_connected() && !time_reached(usb_timeout)) {
        sleep_ms(10);
    }

    printf("\n[transport_wifi] boot\n");
    printf("[transport_wifi] start wifi init\n");
    fflush(stdout);

    if (!rx_buffer || rx_buffer->length == 0) {
        printf("[transport_wifi] invalid rx buffer\n");
        fflush(stdout);
        return false;
    }

    if (rx_buffer->data == NULL) {
        rx_buffer->data = malloc(rx_buffer->length * sizeof(char));
        if (rx_buffer->data == NULL) {
            return false;
        }
        memset(rx_buffer->data, 0, rx_buffer->length * sizeof(char));
    }

    rx_buffer->ready = false;

    wifi_handler.on_receive = wifi_receive_callback;
    wifi_handler.on_receive_context = rx_buffer;

    bool ok = wifi_handler_init(&wifi_handler);
    printf("[transport_wifi] wifi init result: %d\n", ok);
    fflush(stdout);
    return ok;
}

bool transport_wait_until_connected(void)
{
    while (!wifi_handler_has_clients(&wifi_handler)) {
        wifi_handler_poll(&wifi_handler);
        sleep_ms(10);
    }

    return true;
}

void transport_poll_rx(transport_rx_buffer_t *rx_buffer)
{
    if (!rx_buffer) {
        return;
    }

    rx_buffer->ready = false;
    wifi_handler_poll(&wifi_handler);

    uint8_t byte = 0;
    while (rx_queue_pop(&byte)) {
        transport_push_rx_byte(rx_buffer, byte);

        if (rx_buffer->ready) {
            break;
        }
    }
}

size_t transport_write(char *data, size_t len)
{
    wifi_handler_poll(&wifi_handler);
    size_t written = wifi_handler_send_data(&wifi_handler, (const uint8_t*)data, len);
    wifi_handler_poll(&wifi_handler);
    return written;
}
