#ifndef __CAN_UTILS_H
#define __CAN_UTILS_H

#include <zephyr/device.h>

// Accept frame with any ID
#define CAN_FILTER_MASK 0x0

// No flags, CAN working with standard 11-bit id frames
#define CAN_FILTER_FLAGS 0x0
#define CAN_FRAME_FLAGS  0x0

// Configure CAN filter and start CAN
int setup_can_device();

// Send given number of bytes from data in CAN frame
int send_can_frame(int id, uint8_t *data, size_t size);

// Send end of transmission frame to node
int send_control_frame(int eot_frame_id);

#endif /* __CAN_UTILS_H */
