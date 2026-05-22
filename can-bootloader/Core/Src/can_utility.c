#include "can_utility.h"
#include "string.h"

volatile uint8_t new_user_app_buffer[USER_APP_BUFFER_SIZE] = { 0x0 };
volatile int write_offset = 0;
volatile int write_ready = 0;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  CAN_RxHeaderTypeDef rxHeader;
  uint8_t data[8];

  // Early return if buffer is full
  if (write_offset >= USER_APP_BUFFER_SIZE) {
    return;
  }

  // Get frame payload
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, data);

  if (rxHeader.StdId == CAN_FRAME_APP_FRAGMENT_ID) {
    // New app fragment received

    if (rxHeader.DLC == 8) {

      memcpy((void*)(new_user_app_buffer + write_offset), data, 8);
      write_offset += 8;

    } else {

      uint8_t buffer[8] = { 0x0 };
      memset(buffer, 0xFF, 8);

      memcpy(buffer, data, rxHeader.DLC);
      memcpy((void*)(new_user_app_buffer + write_offset), buffer, 8);
      write_offset += rxHeader.DLC;

    }

  } else if (rxHeader.StdId == CAN_FRAME_APP_TX_END_ID) {
    // App transmition finished

    write_ready = 1;

  }
}
