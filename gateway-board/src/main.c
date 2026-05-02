#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>

#include "user-application.h"

LOG_MODULE_REGISTER(gateway, LOG_LEVEL_DBG);

#define CAN_SEND_INTERVAL_MS 100

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct device *can_dev = DEVICE_DT_GET(DT_ALIAS(can0));

void rx_callback(const struct device *dev, struct can_frame *frame, void *user_data) 
{
    LOG_INF("Received msg %d", frame->id);
}

void tx_callback(const struct device *dev, int error, void *user_data)
{
    if (error) {
        LOG_ERR("TX error %d", error);
    }
}

int main()
{
    if (!device_is_ready(led.port)) {
        LOG_ERR("LED device not ready");
        return 1;
    }

    if (!device_is_ready(can_dev)) {
        LOG_ERR("CAN device not ready");
        return 1;
    }

    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED as output, error %d", ret);
        return 1;
    }

    struct can_filter filter = {
        .id = 0,
        .mask = 0,
        .flags = 0
    };

    if (!device_is_ready(can_dev)) {
        LOG_ERR("CAN not ready\n");
    }

    ret = can_add_rx_filter(can_dev, &rx_callback, NULL, &filter);
    if (ret < 0) {
        LOG_ERR("Failed to set CAN filter, error %d", ret);
        return 1;
    }

    ret = can_start(can_dev);
    if (ret < 0) {
        LOG_ERR("Failed to start CAN, error %d", ret);
        return 1;
    }

    LOG_INF("Start sending app");

    int idx = 0;
    const int can_payload_size = 8;
    while (idx + can_payload_size <= user_application_bin_len) {
        struct can_frame frame = {
            .id = 0x10, // arbitrary chosen
            .flags = 0,
            .dlc = can_payload_size,
        };
        memcpy(frame.data, user_application_bin + idx, can_payload_size);

        ret = can_send(can_dev, &frame, K_FOREVER, &tx_callback, NULL);
        if (ret < 0) {
            LOG_ERR("Failed to send CAN frame, error %d", ret);
            return 1;
        } else {
            LOG_INF("Sent (%d / %d) [ %02x %02x %02x %02x %02x %02x %02x %02x ]",
                    idx + 8, user_application_bin_len,
                    frame.data[0], frame.data[1], frame.data[2],
                    frame.data[3], frame.data[4], frame.data[5],
                    frame.data[6], frame.data[7]);
        }

        idx += can_payload_size;
    }

    struct can_frame frame = {
        .id = 0x10,
        .flags = 0,
        .dlc = user_application_bin_len - idx
    };
    for (int i = 0; i < frame.dlc; ++i) {
        frame.data[i] = user_application_bin[idx + i];
    }

    ret = can_send(can_dev, &frame, K_FOREVER, &tx_callback, NULL);
    if (ret < 0) {
        LOG_ERR("Failed to send CAN frame, error %d", ret);
        return 1;
    } else {
        LOG_INF("Sent (%d / %d)", user_application_bin_len, user_application_bin_len);
    }

    frame.id = 0x11;
    ret = can_send(can_dev, &frame, K_FOREVER, &tx_callback, NULL);
    if (ret < 0) {
        LOG_ERR("Failed to send CAN frame, error %d", ret);
        return 1;
    } else {
        LOG_INF("Sent EOT frame");
    }

    LOG_INF("Flashing successful\nGoing idle...");

    while (true) {
        gpio_pin_toggle_dt(&led);
        k_msleep(CAN_SEND_INTERVAL_MS);
    }
    
    return 0;
}
