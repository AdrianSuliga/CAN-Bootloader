#include "can-utils.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CanUtils, LOG_LEVEL_DBG);

K_SEM_DEFINE(can_ctrl_frame_sem, 0, 1);

static const struct device *can_dev = DEVICE_DT_GET(DT_ALIAS(can0));
static volatile uint32_t current_can_control_frame = UINT32_MAX;

static void rx_callback(const struct device *dev, struct can_frame *frame, void *user_data) 
{
    LOG_INF("Received CAN frame with ID 0x%02X, comparing with global ID 0x%02X", frame->id, current_can_control_frame);

    if (frame->id != current_can_control_frame) {
        return;
    }

    if (frame->dlc != 1) {
        return;
    }

    if (frame->data[0] != BOOTLOADER_COMMAND_ACK) {
        return;
    }

    k_sem_give(&can_ctrl_frame_sem);
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

int send_control_frame(int ctrl_frame_id, uint8_t command)
{
    return send_can_frame(ctrl_frame_id, &command, sizeof(uint8_t));
}

int send_wait_control_frame(int ctrl_frame_id, uint8_t command)
{
    // Set global CAN CTRL frame ID
    current_can_control_frame = ctrl_frame_id;

    // Send CAN CTRL frame
    int ret = send_control_frame(ctrl_frame_id, command);
    if (ret != 0) {
        LOG_ERR("Failed to send CTRL frame to node, error %d", ret);
        return ret;
    }

    // Wait for CAN CTRL frame back
    ret = k_sem_take(&can_ctrl_frame_sem, K_SECONDS(CAN_CTRL_MSG_TIMEOUT));
    if (ret != 0) {
        LOG_ERR("Failed to receive CTRL message back, node in unknown state, error %d", ret);
    } else {
        LOG_INF("Received CTRL message, bootloader success confirmed");
    }

    // Reset global state
    current_can_control_frame = -1;

    return ret;
}
