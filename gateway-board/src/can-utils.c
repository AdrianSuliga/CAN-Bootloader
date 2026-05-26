#include "can-utils.h"
#include "wifi-utils.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CanUtils, LOG_LEVEL_DBG);

static void rx_callback(const struct device *dev, struct can_frame *frame, void *user_data) 
{
    LOG_INF("Received msg %d", frame->id);
}

static void tx_callback(const struct device *dev, int error, void *user_data)
{
    if (error) {
        LOG_ERR("TX error %d", error);
    }
}

int setup_can_device(const struct device *can_dev)
{
    // Setup CAN filter (accept every frame)
    struct can_filter filter = {
        .id = 0,
        .mask = CAN_FILTER_MASK,
        .flags = CAN_FILTER_FLAGS
    };

    // Add filter and RX callback
    int ret = can_add_rx_filter(can_dev, &rx_callback, NULL, &filter);
    if (ret < 0) {
        LOG_ERR("Failed to set CAN filter, error %d", ret);
        return ret;
    }

    // Start CAN device
    return can_start(can_dev);
}

int send_can_frame(const struct device *can_dev, int id, uint8_t *data, size_t size)
{
    // Verify data size
    if (!(0 <= size && size <= 8)) {
        LOG_ERR("Incorrect frame size, error %d", size);
        return 1;
    }

    // Pack data into CAN frame
    struct can_frame frame = {
        .id = id,
        .flags = CAN_FRAME_FLAGS,
        .dlc = size
    };

    memcpy(frame.data, data, size);

    // Send CAN frame 
    return can_send(can_dev, &frame, K_FOREVER, &tx_callback, NULL);
}

int send_user_application_buffer(const struct device *can_dev)
{
    LOG_INF("Start sending app");

    int ret, idx = 0;
    const int can_payload_size = 8;

    // Go through user_application_buffer and send
    // 8-byte fragments via CAN
    while (idx + can_payload_size <= rx_buffer_app_size) {

        ret = send_can_frame(can_dev, CAN_FRAME_APP_FRAGMENT_ID,
                             rx_buffer + idx, can_payload_size);
        if (ret < 0) {
            LOG_ERR("Failed to send CAN frame, error %d", ret);
            return ret;
        }
        
        LOG_INF("Sent (%d / %d) [ %02x %02x %02x %02x %02x %02x %02x %02x ]",
                idx + 8, rx_buffer_app_size,
                rx_buffer[idx],     rx_buffer[idx + 1],
                rx_buffer[idx + 2], rx_buffer[idx + 3],
                rx_buffer[idx + 4], rx_buffer[idx + 5],
                rx_buffer[idx + 6], rx_buffer[idx + 7]);

        idx += can_payload_size;
    }

    // If there are bytes left, send them as well
    if (rx_buffer_app_size - idx > 0) {
        ret = send_can_frame(can_dev, CAN_FRAME_APP_FRAGMENT_ID,
                             rx_buffer + idx, rx_buffer_app_size - idx);
        if (ret < 0) {
            LOG_ERR("Failed to send CAN frame, error %d", ret);
            return ret;
        }

        LOG_INF("Sent (%d / %d)", rx_buffer_app_size, rx_buffer_app_size);
    }
    
    // Send End of Transmission frame
    ret = send_can_frame(can_dev, CAN_FRAME_APP_TX_END_ID, NULL, 0);
    if (ret < 0) {
        LOG_ERR("Failed to send CAN frame, error %d", ret);
        return ret;
    }
    
    LOG_INF("Sent EOT frame");
    LOG_INF("Flashing successful");
    
    return 0;
}
