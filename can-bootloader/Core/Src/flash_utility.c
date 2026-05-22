#include "flash_utility.h"
#include "can_utility.h"
#include "string.h"

HAL_StatusTypeDef Flash_Erase_User_App_Slot(enum UserApplicationSlot slot)
{
  // Validate caller wants to erase valid slot
  if (slot != USER_APP_SLOT_1 && slot != USER_APP_SLOT_2) {
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
  erase.Sector = slot == USER_APP_SLOT_1 ?
                 USER_APP_SLOT_1_START : USER_APP_SLOT_2_START;
  erase.NbSectors = slot == USER_APP_SLOT_2 ?
                    USER_APP_SLOT_1_SECTOR_CNT : USER_APP_SLOT_2_SECTOR_CNT;

  // Erase user application slots
  HAL_StatusTypeDef result = HAL_FLASHEx_Erase(&erase, &error);

  // Lock Flash again after completing operation
  HAL_FLASH_Lock();

  // Enable interrupts after Flash critical section
  __enable_irq();

  return result;
}

HAL_StatusTypeDef Flash_Write_User_App(enum UserApplicationSlot slot)
{
  // Validate caller wants to erase valid slot
  if (slot != USER_APP_SLOT_1 && slot != USER_APP_SLOT_2) {
    return HAL_ERROR;
  }

  HAL_StatusTypeDef result;
  uint32_t base_address = slot == USER_APP_SLOT_1 ?
                          USER_APP_SLOT_1_ADDR : USER_APP_SLOT_2_ADDR;

  // Disable interrupts for Flash critical section
  __disable_irq();

  // Unlock Flash memory
  HAL_FLASH_Unlock();

  // Loop through buffer and save its content to user application slot
  for (int i = 0; i < USER_APP_BUFFER_SIZE; i += 4) {
    
    // Flash memory is arranged in 32-bit words
    uint32_t word_size = sizeof(uint32_t);
    uint32_t word1;
    
    memcpy(&word1, (void*)(new_user_app_buffer + i), word_size);

    // Clear Flash flags
    FLASH_CLEAR_FLAGS();

    // Wait for Flash operations to complete before the write
    FLASH_WaitForLastOperation(50000);

    // Program one 32-bit word of new user application
    result = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, base_address + i, word1);
    if (result != HAL_OK) {
      __enable_irq();
      HAL_FLASH_Lock();
      return result;
    }

  }

  // Lock Flash again after completing operation
  HAL_FLASH_Lock();

  // Enable interrupts after Flash critical section
  __enable_irq();

  // Reset buffer state
  write_ready = 0;
  write_offset = 0;

  memset((void*)new_user_app_buffer, 0xFF, USER_APP_BUFFER_SIZE);

  return HAL_OK;
}
