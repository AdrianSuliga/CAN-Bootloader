#ifndef __WIFI_UTILS_H
#define __WIFI_UTILS_H

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#define WIFI_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

#define WIFI_SSID "NieRyszard"
#define WIFI_PSK "jestnatrello"

#define WIFI_CONNECT_TIMEOUT 30
#define MQTT_CONNECT_TIMEOUT 30

#define MQTT_CLIENT "zephyr_bootloader_mqtt_client"
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_BROKER_PORT 1883
#define MQTT_PUBLISH_TOPIC "system/gateway_board/publish/state"
#define MQTT_SUBSCRIBE_TOPIC "system/gateway_board/subscribe/new_app"

#define MQTT_MESSAGE_RX_BUFFER_SIZE 16384
#define MQTT_MESSAGE_TX_BUFFER_SIZE 256

// Received new firmware
// Declared in wifi-utils.h,
// defined in wifi-utils.c
extern struct k_sem mqtt_msg_app_received;

// Device connected to WiFi
// Declared in wifi-utils.h,
// defined in wifi-utils.c
extern atomic_t wifi_ready;

// Connection to MQTT broker established
// Declared in wifi-utils.h,
// defined in wifi-utils.c
extern atomic_t mqtt_ready;

extern uint8_t tx_buffer[MQTT_MESSAGE_TX_BUFFER_SIZE];
extern uint8_t rx_buffer[MQTT_MESSAGE_RX_BUFFER_SIZE];
extern uint32_t rx_buffer_app_size;

// Establish WiFi connection, blocking
int setup_wifi();

// Establish MQTT connection, blocking
int setup_mqtt();

// Poll MQTT socket, must be called at least
// once to get MQTT connection
int poll_mqtt();

#endif /* __WIFI_UTILS_H */
