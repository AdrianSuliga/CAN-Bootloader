#include "crc_utility.h"
#include "stm32f7xx_hal.h"
#include "string.h"

extern CRC_HandleTypeDef hcrc;

void CRC_Checksum_Initialize() {
  __HAL_CRC_DR_RESET(&hcrc);
}

void CRC_Checksum_Update(const uint8_t *data, uint32_t size) {
  for (uint32_t i = 0; i < size; i += sizeof(uint32_t)) {
    uint32_t word;
    memcpy(&word, data + i, sizeof(uint32_t));

    // Before feeding the words to the peripheral, we need to first reflect them for compatibility
    // with IEEE CRC-32
    hcrc.Instance->DR = __RBIT(word);
  }
}

uint32_t CRC_Checksum_Finalize() {
  // For compatibility with IEEE CRC-32, we must first reflect the retrieved checksum, then invert
  // its bytes
  uint32_t crc_inv = __RBIT(hcrc.Instance->DR);
  return ~crc_inv;
}
