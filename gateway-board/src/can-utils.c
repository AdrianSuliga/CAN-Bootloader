#include "can-utils.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CanUtils, LOG_LEVEL_DBG);

static const struct device *can_dev = DEVICE_DT_GET(DT_ALIAS(can0));

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

int setup_can_device()
{
    if (!device_is_ready(can_dev)) {
        LOG_ERR("CAN device not ready");
        return 1;
    }

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

int send_can_frame(int id, uint8_t *data, size_t size)
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

int send_control_frame(int eot_frame_id)
{
    return send_can_frame(eot_frame_id, NULL, 0);
}
