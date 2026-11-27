#include <string.h>

#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/async_context.h"

#include "hal/bt_spp/bl_spp.h"


#define SPP_SERVICE_RECORD_HANDLE 0x10001
#define SPP_DEFAULT_PIN "0000"

static btstack_spp_t *spp = NULL;
static uint8_t spp_service_buffer[150];
static btstack_packet_callback_registration_t hci_event_callback_registration;


static void spp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    if (spp == NULL) {
        return;
    }

    bd_addr_t address;
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)) {
                case HCI_EVENT_PIN_CODE_REQUEST:
                    hci_event_pin_code_request_get_bd_addr(packet, address);
                    gap_pin_code_response(address, spp->pin ? spp->pin : SPP_DEFAULT_PIN);
                    break;

                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                    hci_event_user_confirmation_request_get_bd_addr(packet, address);
                    gap_ssp_confirmation_response(address);
                    break;

                case RFCOMM_EVENT_INCOMING_CONNECTION:
                    if (spp->connected) {
                        rfcomm_decline_connection(rfcomm_event_incoming_connection_get_rfcomm_cid(packet));
                    } else {
                        rfcomm_accept_connection(rfcomm_event_incoming_connection_get_rfcomm_cid(packet));
                    }
                    break;

                case RFCOMM_EVENT_CHANNEL_OPENED:
                    if (rfcomm_event_channel_opened_get_status(packet) == 0) {
                        spp->rfcomm_cid = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
                        spp->max_frame_size = rfcomm_event_channel_opened_get_max_frame_size(packet);
                        spp->connected = true;
                    }
                    break;

                case RFCOMM_EVENT_CHANNEL_CLOSED:
                    if (rfcomm_event_channel_closed_get_rfcomm_cid(packet) == spp->rfcomm_cid) {
                        spp->connected = false;
                        spp->rfcomm_cid = 0;
                        spp->max_frame_size = 0;
                    }
                    break;

                default:
                    break;
            }
            break;

        case RFCOMM_DATA_PACKET:
            if (spp->on_receive != NULL) {
                spp->on_receive(spp->on_receive_context, packet, size);
            }
            break;

        default:
            break;
    }
}


bool btstack_spp_init(btstack_spp_t *handler)
{
    if (handler == NULL) {
        return false;
    }
    if (handler->init_done) {
        return true;
    }
    if (spp != NULL) {
        return false;
    }

    if (!cyw43_is_initialized(&cyw43_state) && cyw43_arch_init()) {
        return false;
    }

    spp = handler;
    handler->connected = false;
    handler->rfcomm_cid = 0;
    handler->max_frame_size = 0;
    uint8_t channel = handler->rfcomm_channel ? handler->rfcomm_channel : 1;

    hci_event_callback_registration.callback = &spp_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    l2cap_init();
    rfcomm_init();
    rfcomm_register_service(spp_packet_handler, channel, 0xffff);

    sdp_init();
    memset(spp_service_buffer, 0, sizeof(spp_service_buffer));
    spp_create_sdp_record(spp_service_buffer, SPP_SERVICE_RECORD_HANDLE, channel, "PICO SPP");
    sdp_register_service(spp_service_buffer);

    gap_set_local_name(handler->name ? handler->name : "PICO SPP 00:00:00:00:00:00");
    gap_discoverable_control(1);
    gap_connectable_control(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);

    hci_power_control(HCI_POWER_ON);
    handler->init_done = true;
    return true;
}


void btstack_spp_deinit(btstack_spp_t *handler)
{
    if (handler == NULL || !handler->init_done || handler != spp) {
        return;
    }
    hci_power_control(HCI_POWER_OFF);
    hci_remove_event_handler(&hci_event_callback_registration);
    cyw43_arch_deinit();
    handler->connected = false;
    handler->init_done = false;
    spp = NULL;
}


bool btstack_spp_is_connected(const btstack_spp_t *handler)
{
    return handler != NULL && handler->init_done && handler->connected;
}


size_t btstack_spp_send(btstack_spp_t *handler, const uint8_t *data, size_t len)
{
    if (!btstack_spp_is_connected(handler) || data == NULL) {
        return 0;
    }

    async_context_t *context = cyw43_arch_async_context();
    size_t sent = 0;
    absolute_time_t timeout = make_timeout_time_ms(BTSTACK_SPP_SEND_TIMEOUT_MS);

    while (sent < len && handler->connected) {
        bool progress = false;

        async_context_acquire_lock_blocking(context);
        if (handler->connected && rfcomm_can_send_packet_now(handler->rfcomm_cid)) {
            size_t chunk = len - sent;
            if (chunk > handler->max_frame_size) {
                chunk = handler->max_frame_size;
            }
            if (rfcomm_send(handler->rfcomm_cid, (uint8_t *)&data[sent], (uint16_t)chunk) == 0) {
                sent += chunk;
                progress = true;
            }
        }
        async_context_release_lock(context);

        if (progress) {
            timeout = make_timeout_time_ms(BTSTACK_SPP_SEND_TIMEOUT_MS);
        } else if (time_reached(timeout)) {
            break;
        } else {
            sleep_us(200);
        }
    }
    return sent;
}
