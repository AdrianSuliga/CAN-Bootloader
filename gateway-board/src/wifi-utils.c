#include "wifi-utils.h"
#include "can-utils.h"

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(WifiUtils, LOG_LEVEL_DBG);

/* Semaphores for internal state signaling */
K_SEM_DEFINE(wifi_ready_flag, 0, 1);
K_SEM_DEFINE(mqtt_ready_flag, 0, 1);

/* Atomic flags for state machine from main.c */
atomic_t wifi_ready = ATOMIC_INIT(0x0);
atomic_t mqtt_ready = ATOMIC_INIT(0x0);

/* MQTT buffers */
static uint8_t rx_buffer[MQTT_MESSAGE_RX_BUFFER_SIZE] = { 0x0 };
static uint8_t tx_buffer[MQTT_MESSAGE_TX_BUFFER_SIZE] = { 0x0 };

/* WiFi and MQTT internal structs */
static struct net_mgmt_event_callback wifi_callback;
static struct sockaddr_in broker;
static struct mqtt_client client;
static struct zsock_pollfd fds;

/* WiFi internal functions */
static void update_state_on_wifi_connect();
static void update_state_on_wifi_disconnect();
static void fill_wifi_connect_params(struct wifi_connect_req_params *params);
static void wifi_handler(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface);

/* MQTT internal functions */
static void update_state_on_mqtt_connect();
static void update_state_on_mqtt_disconnect();
static int subscribe();
static int server_resolve();
static void fill_mqtt_client_params();
static void mqtt_handler(struct mqtt_client *client, const struct mqtt_evt *evt);

/* CAN internal functions */
static int send_rx_buffer(int firmware_frame_id, int rx_buffer_current_size);
static int send_rx_buffer_protected(int control_frame_id, int firmware_frame_id, int rx_buffer_current_size);
static int flash_new_firmware(struct mqtt_client *client, const struct mqtt_evt *evt);

/* ***************** */
/* MODULE PUBLIC API */
/* ***************** */

/* WiFi setup, first point of interaction with this module */
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

/* MQTT setup, second point of interaction with this module */
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
    fds.events = ZSOCK_POLLIN;

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

/* MQTT poll, should be called periodically */
int poll_mqtt()
{
    int err = zsock_poll(&fds, 1, mqtt_keepalive_time_left(&client));
    if (err < 0) {
        LOG_ERR("Error when calling poll, error %d", err);
        return err;
    }

    err = mqtt_live(&client);
    if (err != 0 && err != -EAGAIN) {
        LOG_ERR("Error when calling mqtt_live, error %d", err);
        return err;
    }

    if ((fds.revents & ZSOCK_POLLIN) == ZSOCK_POLLIN) {
        err = mqtt_input(&client);
        if (err) {
            LOG_ERR("Error when calling mqtt_input, error %d", err);
            return err;
        }

        fds.revents = 0;
    }

    return 0;
}

/* ****************** */
/* INTERNAL FUNCTIONS */
/* ****************** */

/* ----------------------- */
/* Internal WiFi Functions */
/* ----------------------- */

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

/* ----------------------- */
/* Internal MQTT Functions */
/* ----------------------- */

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

static int server_resolve()
{
    int err;

    struct zsock_addrinfo *result;
    struct zsock_addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };

    err = zsock_getaddrinfo(MQTT_BROKER, NULL, &hints, &result);
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

    zsock_freeaddrinfo(result);

    return err;
}

static int consume_payload_on_error(struct mqtt_client *client)
{
    LOG_INF("Consuming payload due to error");

    int read = 1;
    while (read > 0) {
        read = mqtt_read_publish_payload(client, rx_buffer, MQTT_MESSAGE_RX_BUFFER_SIZE);
        
        if (read == -EAGAIN) {
            read = 1;
            continue;
        } else if (read < 0) {
            LOG_ERR("Error while consuming payload, error %d", read);
            return read;
        }

        memset(rx_buffer, 0, MQTT_MESSAGE_RX_BUFFER_SIZE);
    }

    return read;
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

            int err = flash_new_firmware(client, evt);
            if (err) {
                LOG_ERR("Failed to flash new application, error %d", err);
                err = consume_payload_on_error(client);
                if (err) {
                    LOG_ERR("MQTT payload failed to be consumed, error %d", err);
                }
            }
            break;

        case MQTT_EVT_SUBACK:
        case MQTT_EVT_PINGRESP:
            break;
        
        default:
            LOG_WRN("Unknown MQTT event, %d", evt->type);
            break;
    }
}

/* ---------------------- */
/* Internal CAN functions */
/* ---------------------- */

static int send_rx_buffer(int firmware_frame_id, int rx_buffer_current_size)
{
    LOG_INF("Start sending app");

    int ret, idx = 0;
    const int can_payload_size = 8;

    // Go through user_application_buffer and send
    // 8-byte fragments via CAN
    while (idx + can_payload_size <= rx_buffer_current_size) {

        ret = send_can_frame(firmware_frame_id, rx_buffer + idx, can_payload_size);
        if (ret < 0) {
            LOG_ERR("Failed to send CAN frame, error %d", ret);
            return ret;
        }
        
        LOG_INF("Sent (%d / %d) [ %02x %02x %02x %02x %02x %02x %02x %02x ]",
                idx + 8, rx_buffer_current_size,
                rx_buffer[idx],     rx_buffer[idx + 1],
                rx_buffer[idx + 2], rx_buffer[idx + 3],
                rx_buffer[idx + 4], rx_buffer[idx + 5],
                rx_buffer[idx + 6], rx_buffer[idx + 7]);

        idx += can_payload_size;
    }

    // If there are bytes left, send them as well
    if (rx_buffer_current_size - idx > 0) {
        ret = send_can_frame(firmware_frame_id, rx_buffer + idx, rx_buffer_current_size - idx);
        if (ret < 0) {
            LOG_ERR("Failed to send CAN frame, error %d", ret);
            return ret;
        }

        LOG_INF("Sent (%d / %d)", rx_buffer_current_size, rx_buffer_current_size);
    }
    
    LOG_INF("Flashing of buffer successful");
    
    return 0;
}

static int send_rx_buffer_protected(int control_frame_id, int firmware_frame_id, int rx_buffer_current_size)
{
    int err = send_rx_buffer(firmware_frame_id, rx_buffer_current_size);
    if (err) {
        LOG_ERR("Failed to send firmware, aborting. Error %d", err);
        
        // If continuing flashing is not possible, sent EOT frame so
        // bootloader can recover to previous app image
        int ret = send_control_frame(control_frame_id);
        if (ret) {
            LOG_ERR("Failed to end transmission, node left in unknown state, error %d", ret);
            return ret;
        }

        return err;
    }

    return 0;
}

static int flash_new_firmware(struct mqtt_client *client, const struct mqtt_evt *evt)
{
    int ret;
    memset(rx_buffer, 0, MQTT_MESSAGE_RX_BUFFER_SIZE);

    // How many bytes were read to buffer
    uint32_t buffer_read = 0;
    // How many bytes were read from payload
    uint32_t payload_read = 0;
    // Total payload length
    uint32_t payload_len = evt->param.publish.message.payload.len;

    LOG_INF("Incoming payload size: %d", payload_len);

    // First 2 bytes of MQTT payload are reserved for Control Frame ID
    int control_frame_id = 0;
    ret = mqtt_read_publish_payload(client, &control_frame_id, 2);
    if (ret != 2) {
        LOG_ERR("Failed to read Control Frame ID, malformed MQTT payload, error %d", ret);
        return ret;
    }
    payload_read += ret;

    // Next 2 bytes of MQTT payload are reserved for Firmware Frame ID
    int firmware_frame_id = 0;
    ret = mqtt_read_publish_payload(client, &firmware_frame_id, 2);
    if (ret != 2) {
        LOG_ERR("Failed to read Firmware Frame ID, malformed MQTT payload, error %d", ret);
        return ret;
    }
    payload_read += ret;

    LOG_INF("CAN frames read - [%02X] [%02X]", control_frame_id, firmware_frame_id);
    LOG_INF("Proceeding with firmware flashing");

    // TODO: implement control functionality, design TBD
    // Control frame so app jumps to bootloader
    // int err = send_control_frame(control_frame_id);
    // if (err) {
    //     LOG_ERR("Failed to end transmission, node left in unknown state, error %d", err);
    //     return err;
    // }

    // Give node some time to jump from app to bootloader
    k_sleep(K_SECONDS(1));

    // Process payload in a loop
    while (payload_read < payload_len) {
        int n = mqtt_read_publish_payload(client, rx_buffer + buffer_read,
                                          MQTT_MESSAGE_RX_BUFFER_SIZE - buffer_read);
        if (n == -EAGAIN) {
            k_sleep(K_SECONDS(1));
            continue;
        } else if (n < 0) {
            LOG_ERR("Failed to call mqtt_read_publish_payload, error %d", n);
            break;
        }

        payload_read += n;
        buffer_read += n;

        LOG_INF("Read MQTT payload (%d / %d), RX buffer (%d / %d)",
                payload_read, payload_len,
                buffer_read, MQTT_MESSAGE_RX_BUFFER_SIZE);

        // If buffer is full, send it to bootloader
        if (buffer_read == MQTT_MESSAGE_RX_BUFFER_SIZE) {
            ret = send_rx_buffer_protected(control_frame_id, firmware_frame_id, MQTT_MESSAGE_RX_BUFFER_SIZE);
            if (ret) {
                LOG_ERR("Flashing CAN RX buffer failed, node left in unknown state, error %d", ret);
                return ret;
            }

            // Reset buffer state
            buffer_read = 0;
            memset(rx_buffer, 0, MQTT_MESSAGE_RX_BUFFER_SIZE);
        }
    }

    // If all of payload was read, bytes remaining in buffer have to be sent
    if (payload_read == payload_len) {
        LOG_INF("Received full firmware image: %d bytes", payload_read);

        ret = send_rx_buffer_protected(control_frame_id, firmware_frame_id, buffer_read);
        if (ret) {
            LOG_ERR("Flashing CAN RX buffer failed, node left in unknown state, error %d", ret);
            return ret;
        }

        // Reset buffer state
        buffer_read = 0;
        memset(rx_buffer, 0, MQTT_MESSAGE_RX_BUFFER_SIZE);
    
        // Control frame so bootloader knows to jump to newly flashed app
        ret = send_control_frame(control_frame_id);
        if (ret) {
            LOG_ERR("Failed to end transmission, node left in unknown state, error %d", ret);
            return ret;
        }
    } else {
        LOG_ERR("Failed to read full payload");
        return 1;
    }

    return 0;
}
