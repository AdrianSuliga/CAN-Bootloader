#ifndef __CAN_UTILS_H
#define __CAN_UTILS_H

#include <zephyr/device.h>

// Accept frame with any ID
#define CAN_FILTER_MASK 0x0

// No flags, CAN working with standard 11-bit id frames
#define CAN_FILTER_FLAGS 0x0
#define CAN_FRAME_FLAGS  0x0

// CAN frame ID for frames containing 
// new user application fragments
#define CAN_FRAME_APP_FRAGMENT_ID 0x10

// CAN frame ID for frames indicating all
// fragments of user applications were sent
#define CAN_FRAME_APP_TX_END_ID 0x11

// Configure CAN filter and start CAN
int setup_can_device(const struct device *can_dev);

// Send given number of bytes from data in CAN frame
int send_can_frame(const struct device *can_dev, int id,
                   uint8_t *data, size_t size);

// Send all bytes in user_application_buffer via CAN frames
int send_user_application_buffer(const struct device *can_dev);

#endif /* __CAN_UTILS_H */
