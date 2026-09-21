#include "flash_utility.h"
#include "boot_utility.h"
#include "can_utility.h"
#include "string.h"

HAL_StatusTypeDef Flash_Erase_TargetSlot()
{
  // Validate caller wants to erase valid slot
  if (target_slot != USER_APP_SLOT_1 && target_slot != USER_APP_SLOT_2) {
    return HAL_ERROR;
  }

  // Disable interrupts for Flash critical section
  __disable_irq();

  // Unlock Flash memory
  HAL_FLASH_Unlock();

  // Reset Flash flags
  FLASH_CLEAR_FLAGS();

  // Prepare Flash erase structure
  FLASH_EraseInitTypeDef erase;
  uint32_t error;

  erase.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase.VoltageRange = VOLTAGE_RANGE_3;
  erase.Sector = target_slot == USER_APP_SLOT_1 ?
                 USER_APP_SLOT_1_START : USER_APP_SLOT_2_START;
  erase.NbSectors = target_slot == USER_APP_SLOT_1 ?
                    USER_APP_SLOT_1_SECTOR_CNT : USER_APP_SLOT_2_SECTOR_CNT;

  // Erase user application slots
  HAL_StatusTypeDef result = HAL_FLASHEx_Erase(&erase, &error);

  // Lock Flash again after completing operation
  HAL_FLASH_Lock();

  // Enable interrupts after Flash critical section
  __enable_irq();

  return result;
}

HAL_StatusTypeDef Flash_Write_CANRxBuffer(uint32_t offset)
{
  // Validate caller wants to use valid slot
  if (target_slot != USER_APP_SLOT_1 && target_slot != USER_APP_SLOT_2) {
    return HAL_ERROR;
  }

  HAL_StatusTypeDef result;
  uint32_t base_address = target_slot == USER_APP_SLOT_1 ?
                            USER_APP_SLOT_1_ADDR : USER_APP_SLOT_2_ADDR;
  uint32_t write_address = base_address + offset;

  // Disable interrupts for Flash critical section
  __disable_irq();

  // Unlock Flash memory
  HAL_FLASH_Unlock();

  // Loop through buffer and save its content to user application slot
  for (int i = 0; i < CAN_RX_BUFFER_SIZE; i += 4) {
    
    // Flash memory is arranged in 32-bit words
    uint32_t word_size = sizeof(uint32_t);
    uint32_t word;
    
    memcpy(&word, (void*)(can_rx_buffer + i), word_size);

    // Clear Flash flags
    FLASH_CLEAR_FLAGS();

    // Wait for Flash operations to complete before the write
    FLASH_WaitForLastOperation(50000);

    // Program one 32-bit word of new user application
    result = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, write_address + i, word);
    if (result != HAL_OK) {
      HAL_FLASH_Lock();
      __enable_irq();
      return result;
    }

  }

  // Lock Flash again after completing operation
  HAL_FLASH_Lock();

  // Enable interrupts after Flash critical section
  __enable_irq();

  return HAL_OK;
}
