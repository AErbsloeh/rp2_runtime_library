#include "hal/transport/transport.h"
#include "hal/bt_spp/bl_spp.h"
#include "pico/stdio_usb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TRANSPORT_BT_DEVICE_NAME
#define TRANSPORT_BT_DEVICE_NAME "PICO RP2 00:00:00:00:00:00"
#endif

#ifndef TRANSPORT_BT_PIN
#define TRANSPORT_BT_PIN "0000"
#endif

#ifndef TRANSPORT_BT_RFCOMM_CHANNEL
#define TRANSPORT_BT_RFCOMM_CHANNEL 1
#endif

#ifndef TRANSPORT_BT_RX_QUEUE_SIZE
#define TRANSPORT_BT_RX_QUEUE_SIZE 2048
#endif

#ifndef TRANSPORT_BT_USB_WAIT_MS
#define TRANSPORT_BT_USB_WAIT_MS 0
#endif

static uint8_t rx_queue[TRANSPORT_BT_RX_QUEUE_SIZE];
static volatile size_t rx_head = 0;
static volatile size_t rx_tail = 0;
static volatile uint32_t rx_dropped = 0;

static btstack_spp_t spp_handler = {
    .init_done = false,
    .name = TRANSPORT_BT_DEVICE_NAME,
    .pin = TRANSPORT_BT_PIN,
    .rfcomm_channel = TRANSPORT_BT_RFCOMM_CHANNEL,
    .on_receive = NULL,
    .on_receive_context = NULL
};

static bool rx_queue_push(uint8_t byte)
{
    size_t next_head = (rx_head + 1) % TRANSPORT_BT_RX_QUEUE_SIZE;

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
    rx_tail = (rx_tail + 1) % TRANSPORT_BT_RX_QUEUE_SIZE;
    return true;
}

static void bt_receive_callback(void *context, const uint8_t *data, size_t len)
{
    (void)context;

    for (size_t idx = 0; idx < len; idx++) {
        if (!rx_queue_push(data[idx])) {
            rx_dropped++;
        }
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

    absolute_time_t usb_timeout = make_timeout_time_ms(TRANSPORT_BT_USB_WAIT_MS);
    while (!stdio_usb_connected() && !time_reached(usb_timeout)) {
        sleep_ms(10);
    }
    fflush(stdout);

    if (!rx_buffer || rx_buffer->length == 0) {
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

    spp_handler.on_receive = bt_receive_callback;
    spp_handler.on_receive_context = rx_buffer;

    return btstack_spp_init(&spp_handler);
}

bool transport_wait_until_connected(void)
{
    while (!btstack_spp_is_connected(&spp_handler)) {
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
    return btstack_spp_send(&spp_handler, (const uint8_t *)data, len);
}
