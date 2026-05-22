#ifndef __CAN_UTILITY_H
#define __CAN_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f7xx_hal.h"

// CAN frame ID for frames containing 
// new user application fragments
#define CAN_FRAME_APP_FRAGMENT_ID 0x10

// CAN frame ID for frames indicating all
// fragments of user applications were sent
#define CAN_FRAME_APP_TX_END_ID 0x11

// Size of buffer for user application
#define USER_APP_BUFFER_SIZE 8192U

// Declared in can_utility.h, defines in can_utility.c
// Buffer for new user application
extern volatile uint8_t new_user_app_buffer[USER_APP_BUFFER_SIZE];

// Declared in can_utility.h, defined in can_utility.c
// Current writing position
extern volatile int write_offset;

// Declared in can_utility.h, defined in can_utility.c
// Indicate that new user application is ready
extern volatile int write_ready;

// Callback for receiving new CAN frame
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_UTILITY_H */
