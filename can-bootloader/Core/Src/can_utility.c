#include "can_utility.h"
#include "main.h"
#include "stm32f7xx_hal.h"
#include "string.h"

volatile uint8_t new_user_app_buffer[USER_APP_BUFFER_SIZE] = { 0x0 };

volatile int write_offset = 0;
volatile int write_ready  = 0;

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

  if (rxHeader.StdId == CAN_FRAME_FIRMWARE_FRAGMENT_ID) {
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

  } else if (rxHeader.StdId == CAN_FRAME_BOOTLOADER_CTRL_ID) {
    // App transmition finished

    uint8_t command = data[0];

    if (command == BOOTLOADER_COMMAND_FINISH) {
      write_ready = 1;
    } else if (command == BOOTLOADER_COMMAND_ABORT) {
      // TODO initiate jump to previous slot here
      // when 2-slot solution is ready
      write_ready = 1;
    }
  }
}

HAL_StatusTypeDef HAL_CAN_SendControlFrame(CAN_HandleTypeDef *hcan, uint32_t timeout_ms)
{
  CAN_TxHeaderTypeDef TxHeader = {
    .IDE   = CAN_ID_STD,
    .StdId = CAN_FRAME_BOOTLOADER_CTRL_ID,
    .RTR   = CAN_RTR_DATA,
    .DLC   = 1
  };
  uint32_t TxMailbox;

  uint8_t command = BOOTLOADER_COMMAND_ACK;

  HAL_StatusTypeDef res = HAL_CAN_AddTxMessage(hcan, &TxHeader, &command, &TxMailbox);
  if (res != HAL_OK) {
    return res;
  }

  uint32_t start = HAL_GetTick();

  while (HAL_CAN_IsTxMessagePending(hcan, TxMailbox)) {
    if (HAL_GetTick() - start > timeout_ms) {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}
