#ifndef __CAN_UTILITY_H
#define __CAN_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f7xx_hal.h"

// Default timeout for transmitting CAN
// frame, in ms
#define CAN_MAILBOX_TX_DEFAULT_TIMEOUT 100

// CAN frame ID for frames indicating all
// fragments of user applications were sent.
// Has to be manually set for each system node.
#define CAN_FRAME_BOOTLOADER_CTRL_ID ...

// CAN frame ID for frames containing
// new user application fragments.
// Has to be manually set for each system node.
#define CAN_FRAME_FIRMWARE_FRAGMENT_ID ...

// Command messages used in payload of
// bootloader control messages
#define BOOTLOADER_COMMAND_FINISH 1
#define BOOTLOADER_COMMAND_ACK    2
#define BOOTLOADER_COMMAND_ABORT  3

// Size of buffer for user application
#define CAN_RX_BUFFER_SIZE 4096U

// Declared in can_utility.h, defined in can_utility.c
// Buffer for new user application
extern volatile uint8_t can_rx_buffer[CAN_RX_BUFFER_SIZE];

// Declared in can_utility.h, defined in can_utility.c
// Indicate that new user application is ready
extern volatile int app_ready;

// Declared in can_utility.h, defined in can_utility.c
// Indicate that error occured and abort is needed
extern volatile int abort_required;

// Callback for receiving new CAN frame
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);

// Function to send control message with ACK command via CAN
HAL_StatusTypeDef CAN_Send_ControlFrame(CAN_HandleTypeDef *hcan, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_UTILITY_H */
