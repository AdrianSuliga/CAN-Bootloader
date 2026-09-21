#include "can_utility.h"
#include "flash_utility.h"
#include "stm32f7xx_hal.h"
#include "string.h"

volatile uint8_t can_rx_buffer[CAN_RX_BUFFER_SIZE] = { 0x0 };

static volatile uint32_t flash_offset = 0;
static volatile uint32_t can_rx_buffer_offset = 0;

volatile int abort_required = 0;
volatile int app_ready      = 0;

static HAL_StatusTypeDef CAN_Write_RxBuffer(uint32_t buffer_size)
{
  HAL_StatusTypeDef res = Flash_Write_CANRxBuffer(flash_offset);
  if (res != HAL_OK) {
    return res;
  }

  flash_offset += buffer_size;
  can_rx_buffer_offset = 0;
  memset((void*)can_rx_buffer, 0xFF, CAN_RX_BUFFER_SIZE);

  return HAL_OK;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  HAL_StatusTypeDef res;
  CAN_RxHeaderTypeDef rxHeader;
  uint8_t data[8];

  // Get frame payload
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, data);

  switch (rxHeader.StdId) {

    // New app fragment received
    case CAN_FRAME_FIRMWARE_FRAGMENT_ID:

      memcpy((void*)(can_rx_buffer + can_rx_buffer_offset), data, rxHeader.DLC);
      can_rx_buffer_offset += rxHeader.DLC;

      if (can_rx_buffer_offset == CAN_RX_BUFFER_SIZE) {
        res = CAN_Write_RxBuffer(CAN_RX_BUFFER_SIZE);
        if (res != HAL_OK) {
          abort_required;
        }
      }

      break;

    // App transmition finished
    case CAN_FRAME_BOOTLOADER_CTRL_ID:

      // Early return on malformed control frame
      if (rxHeader.DLC != 1) {
        return;
      }

      uint8_t command = data[0];

      if (command == BOOTLOADER_COMMAND_FINISH) {

        // If transmission ended with success,
        // flash what remains in receive buffer
        res = CAN_Write_RxBuffer(can_rx_buffer_offset);
        if (res == HAL_OK) {
          app_ready = 1;
        } else {
          abort_required = 1;
        }

      } else if (command == BOOTLOADER_COMMAND_ABORT) {

        abort_required = 1;

      }

      break;

    // Every other CAN frame is ignored
    default:
      break;
  }
}

HAL_StatusTypeDef CAN_Send_ControlFrame(CAN_HandleTypeDef *hcan, uint32_t timeout_ms)
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
