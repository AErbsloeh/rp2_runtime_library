#include "hal/wifi/wifi_tcp.h"
#include "lwip/dhcp.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>
#include <string.h>

#define MAX_CLIENTS 1

typedef struct {
    struct tcp_pcb *pcb;
    wifi_handler_t *handler;
} wifi_client_t;

static wifi_client_t clients[MAX_CLIENTS] = {0};

static void clear_client(wifi_client_t *client)
{
    if (client) {
        client->pcb = NULL;
        client->handler = NULL;
    }
}

static void set_static_ip(wifi_handler_t* handler)
{
    if (!handler->static_ip || !handler->netmask || !handler->gateway) {
        return;
    }

    struct netif *netif = netif_list;
    dhcp_stop(netif);

    ip4_addr_t ip;
    ip4_addr_t netmask;
    ip4_addr_t gateway;

    ip4addr_aton(handler->static_ip, &ip);
    ip4addr_aton(handler->netmask, &netmask);
    ip4addr_aton(handler->gateway, &gateway);

    netif_set_addr(netif, &ip, &netmask, &gateway);
    netif_set_up(netif);
}

static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    wifi_client_t *client = (wifi_client_t*)arg;

    if (err != ERR_OK) {
        if (p) {
            pbuf_free(p);
        }
        return err;
    }

    if (!p) {
        tcp_arg(tpcb, NULL);
        tcp_recv(tpcb, NULL);
        tcp_err(tpcb, NULL);
        clear_client(client);
        tcp_close(tpcb);
        return ERR_OK;
    }

    if (client && client->handler && client->handler->on_receive) {
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            client->handler->on_receive(
                client->handler->on_receive_context,
                (const uint8_t*)q->payload,
                q->len
            );
        }
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static void tcp_err_callback(void *arg, err_t err)
{
    (void)err;
    clear_client((wifi_client_t*)arg);
}

static err_t tcp_accept_callback(void *arg, struct tcp_pcb *client_pcb, err_t err)
{
    if (err != ERR_OK || client_pcb == NULL) {
        return ERR_VAL;
    }

    wifi_handler_t *handler = (wifi_handler_t*)arg;

    for (size_t idx = 0; idx < MAX_CLIENTS; idx++) {
        if (clients[idx].pcb == NULL) {
            clients[idx].pcb = client_pcb;
            clients[idx].handler = handler;

            tcp_arg(client_pcb, &clients[idx]);
            tcp_recv(client_pcb, tcp_recv_callback);
            tcp_err(client_pcb, tcp_err_callback);

            printf("TCP client connected\n");
            return ERR_OK;
        }
    }

    tcp_abort(client_pcb);
    return ERR_ABRT;
}

bool wifi_handler_init(wifi_handler_t* handler)
{
    if (!handler || !handler->ssid || !handler->password) {
        return false;
    }

    if (cyw43_arch_init_with_country(handler->country)) {
        printf("WIFI init failed\n");
        return false;
    }

    cyw43_arch_enable_sta_mode();

#if LWIP_NETIF_HOSTNAME
    if (handler->hostname && netif_list) {
        netif_list->hostname = handler->hostname;
    }
#endif

    if (cyw43_arch_wifi_connect_timeout_ms(
        handler->ssid,
        handler->password,
        CYW43_AUTH_WPA2_AES_PSK,
        10000
    )) {
        printf("Could not connect to WIFI network %s\n", handler->ssid);
        cyw43_arch_deinit();
        return false;
    }

    if (!handler->use_dhcp) {
        set_static_ip(handler);
    }

    printf("WiFi connected\n");
    if (handler->hostname) {
        printf("Hostname: %s\n", handler->hostname);
    }
    printf("Pico IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    printf("Netmask: %s\n", ip4addr_ntoa(netif_ip4_netmask(netif_list)));
    printf("Gateway: %s\n", ip4addr_ntoa(netif_ip4_gw(netif_list)));

    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        return false;
    }

    err_t bind_err = tcp_bind(pcb, IP_ADDR_ANY, handler->port);
    if (bind_err != ERR_OK) {
        printf("Error binding port %d: %d\n", handler->port, bind_err);
        tcp_close(pcb);
        return false;
    }

    pcb = tcp_listen(pcb);
    if (!pcb) {
        return false;
    }

    tcp_arg(pcb, handler);
    tcp_accept(pcb, tcp_accept_callback);

    handler->init_done = true;
    printf("TCP server listening on port %d\n", handler->port);
    return true;
}

void wifi_handler_poll(wifi_handler_t* handler)
{
    if (handler && handler->init_done) {
        cyw43_arch_poll();
    }
}

bool wifi_handler_has_clients(wifi_handler_t* handler)
{
    (void)handler;

    for (size_t idx = 0; idx < MAX_CLIENTS; idx++) {
        if (clients[idx].pcb != NULL) {
            return true;
        }
    }

    return false;
}

size_t wifi_handler_send_data(wifi_handler_t* handler, const uint8_t *data, size_t len)
{
    if (!handler || !handler->init_done || !data || len == 0 || len > 0xffff) {
        return 0;
    }

    for (size_t idx = 0; idx < MAX_CLIENTS; idx++) {
        if (clients[idx].pcb != NULL) {
            if (tcp_sndbuf(clients[idx].pcb) < len) {
                cyw43_arch_poll();
                if (tcp_sndbuf(clients[idx].pcb) < len) {
                    return 0;
                }
            }

            err_t err = tcp_write(clients[idx].pcb, data, (u16_t)len, TCP_WRITE_FLAG_COPY);
            if (err != ERR_OK) {
                return 0;
            }

            tcp_output(clients[idx].pcb);
            return len;
        }
    }

    return 0;
}

void wifi_handler_broadcast_string(wifi_handler_t* handler, const char *msg)
{
    if (msg) {
        wifi_handler_send_data(handler, (const uint8_t*)msg, strlen(msg));
    }
}

void wifi_handler_broadcast_data(wifi_handler_t* handler, uint8_t *data, uint8_t data_len)
{
    wifi_handler_send_data(handler, data, data_len);
}
