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
static struct pollfd fds;

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

static void wifi_handler(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
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

static void update_state_on_mqtt_connect()
{
    k_sem_give(&mqtt_ready_flag);
    atomic_set(&mqtt_ready, 1);
}

static void update_state_on_mqtt_disconnect()
{
    k_sem_take(&mqtt_ready_flag, K_NO_WAIT);
    atomic_set(&mqtt_ready, 0);
}

static int subscribe()
{
    struct mqtt_topic sub_topic = {
        .topic = {
            .utf8 = MQTT_SUBSCRIBE_TOPIC,
            .size = strlen(MQTT_SUBSCRIBE_TOPIC)
        },
        .qos = MQTT_QOS_0_AT_MOST_ONCE
    };

    const struct mqtt_subscription_list sub_list = {
        .list = &sub_topic,
        .list_count = 1,
        .message_id = 6767
    };

    LOG_INF("Subscribing to topic %s", MQTT_SUBSCRIBE_TOPIC);
    return mqtt_subscribe(&client, &sub_list);
}

static void mqtt_handler(struct mqtt_client *client, const struct mqtt_evt *evt)
{
    switch (evt->type) {
        case MQTT_EVT_CONNACK:
            if (evt->result != 0) {
                LOG_ERR("Unable to connect to broker, error %d", evt->result);
                break;
            }

            subscribe();
            update_state_on_mqtt_connect();
            LOG_INF("MQTT connected");
            break;

        case MQTT_EVT_DISCONNECT:
            update_state_on_mqtt_disconnect();
            LOG_INF("MQTT disconnected");
            break;

        case MQTT_EVT_PUBLISH:
            LOG_INF("Received message on topic %s", evt->param.publish.message.topic.topic.utf8);
            break;
        
        default:
            LOG_WRN("Unknown MQTT event, %d", evt->type);
            break;
    }
}

static int server_resolve()
{
    int err;

    struct addrinfo *result;
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };

    err = getaddrinfo(MQTT_BROKER, NULL, &hints, &result);
    if (err) {
        LOG_ERR("Failed to get address info of %s", MQTT_BROKER);
        return err;
    }

    if (result == NULL) {
        LOG_ERR("Address not found");
        return -ENOENT;
    }

    broker.sin_family = AF_INET;
    broker.sin_port = htons(MQTT_BROKER_PORT);
    broker.sin_addr.s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;

    char addr_str[NET_IPV4_ADDR_LEN];
    net_addr_ntop(AF_INET, &broker.sin_addr, addr_str, sizeof(addr_str));

    LOG_INF("%s resolved as %s", MQTT_BROKER, addr_str);

    freeaddrinfo(result);

    return err;
}

static void fill_mqtt_client_params()
{
    client.broker = &broker;
    client.evt_cb = mqtt_handler;
    client.client_id.utf8 = MQTT_CLIENT;
    client.client_id.size = strlen(MQTT_CLIENT);
    client.password = NULL;
    client.user_name = NULL;
    client.protocol_version = MQTT_VERSION_3_1_1;

    client.rx_buf = rx_buffer;
    client.rx_buf_size = sizeof(rx_buffer);
    client.tx_buf = tx_buffer;
    client.tx_buf_size = sizeof(tx_buffer);

    client.transport.type = MQTT_TRANSPORT_NON_SECURE;
}

int setup_mqtt()
{
    mqtt_client_init(&client);

    int err = server_resolve();
    if (err != 0) {
        LOG_ERR("Failed to resolve broker hostname");
        return err;
    }

    fill_mqtt_client_params();

    err = mqtt_connect(&client);
    if (err != 0) {
        LOG_ERR("Failed to call mqtt_connect, error %d", err);
        return err;
    }

    fds.fd = client.transport.tcp.sock;
    fds.events = POLLIN;

    err = poll_mqtt();
    if (err != 0) {
        LOG_ERR("Failed to poll MQTT socket, error %d", err);
        return err;
    }

    err = k_sem_take(&mqtt_ready_flag, K_SECONDS(MQTT_CONNECT_TIMEOUT));
    if (err != 0) {
        zsock_close(client.transport.tcp.sock);
        mqtt_disconnect(&client, NULL);
    }

    return err;
}

int poll_mqtt()
{
    int err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
    if (err < 1) {
        LOG_ERR("Error when calling poll, error %d", err);
        return err;
    }

    err = mqtt_live(&client);
    if (err != 0 && err != -EAGAIN) {
        LOG_ERR("Error when calling mqtt_live, error %d", err);
        return err;
    }

    if ((fds.revents & POLLIN) == POLLIN) {
        err = mqtt_input(&client);
        if (err) {
            LOG_ERR("Error when calling mqtt_input, error %d", err);
            return err;
        }
    }

    return 0;
}
