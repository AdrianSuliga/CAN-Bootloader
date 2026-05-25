#include "wifi-utils.h"

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(WifiUtils, LOG_LEVEL_DBG);

static uint8_t rx_buffer[MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t payload_buffer[MQTT_PAYLOAD_BUFFER_SIZE];

K_SEM_DEFINE(mqtt_msg_app_received, 0, 1);
K_SEM_DEFINE(wifi_ready_flag, 0, 1);
K_SEM_DEFINE(mqtt_ready_flag, 0, 1);

atomic_t wifi_ready = ATOMIC_INIT(0x0);
atomic_t mqtt_ready = ATOMIC_INIT(0x0);

static struct net_mgmt_event_callback wifi_callback;
static struct sockaddr_in broker;
static struct mqtt_client client;

static void update_state_on_wifi_connect()
{
    k_sem_give(&wifi_ready_flag);
    atomic_set(&wifi_ready, 1);
}

static void update_state_on_wifi_disconnect()
{
    k_sem_take(&wifi_ready_flag, K_NO_WAIT);
    k_sem_take(&mqtt_ready_flag, K_NO_WAIT);
    atomic_set(&wifi_ready, 0);
    atomic_set(&mqtt_ready, 0);
}

void wifi_handler(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
{
    switch (event) {
        case NET_EVENT_WIFI_CONNECT_RESULT:
            struct wifi_status *received_status = (struct wifi_status *)(cb->info);
            if (received_status->conn_status == WIFI_STATUS_CONN_SUCCESS) {
                update_state_on_wifi_connect();
                LOG_INF("Connected to %s", WIFI_SSID);
            } else {
                LOG_INF("Not connected, error: %d", received_status->conn_status);
            }
            break;

        case NET_EVENT_WIFI_DISCONNECT_RESULT:
            update_state_on_wifi_disconnect();
            LOG_INF("Disconnected, error: %d", *((int32_t *)(cb->info)));
            break;

        default:
            LOG_WRN("Unknown WiFi event reached handler %llu", event);
            break;
    }
}

static void fill_wifi_connect_params(struct wifi_connect_req_params *params)
{
    memset(params, 0, sizeof(struct wifi_connect_req_params));

    params->ssid = WIFI_SSID;
    params->ssid_length = strlen(WIFI_SSID);

    params->psk = WIFI_PSK;
    params->psk_length = strlen(WIFI_PSK);

    params->band = 0;
    params->security = WIFI_SECURITY_TYPE_PSK;
}

int setup_wifi()
{
    struct net_if *iface = net_if_get_default();
    if (iface == NULL) {
        LOG_ERR("Net interface not configured");
        return 1;
    }

    struct wifi_connect_req_params wifi_params;

    fill_wifi_connect_params(&wifi_params);

    net_mgmt_init_event_callback(&wifi_callback, wifi_handler, WIFI_EVENTS);
    net_mgmt_add_event_callback(&wifi_callback);

    net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params, sizeof(wifi_params));

    LOG_INF("Params setup, waiting for WiFi connection");

    return k_sem_take(&wifi_ready_flag, K_SECONDS(WIFI_CONNECT_TIMEOUT));
}

int setup_mqtt()
{
    return k_sem_take(&mqtt_ready_flag, K_SECONDS(MQTT_CONNECT_TIMEOUT));
}
