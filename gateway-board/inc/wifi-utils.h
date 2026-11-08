#ifndef __WIFI_UTILS_H
#define __WIFI_UTILS_H

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#define WIFI_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

#define WIFI_CONNECT_TIMEOUT 30
#define MQTT_CONNECT_TIMEOUT 30

// Device connected to WiFi
// Declared in wifi-utils.h,
// defined in wifi-utils.c
extern atomic_t wifi_ready;

// Connection to MQTT broker established
// Declared in wifi-utils.h,
// defined in wifi-utils.c
extern atomic_t mqtt_ready;

// Initialize WiFi callbacks
void init_wifi();

// Establish WiFi connection, blocking
int setup_wifi();

// Establish MQTT connection, blocking
int setup_mqtt();

// Poll MQTT socket, must be called at least
// once to get MQTT connection
int poll_mqtt();

#endif /* __WIFI_UTILS_H */
