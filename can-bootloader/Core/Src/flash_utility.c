#include "flash_utility.h"
#include "boot_utility.h"
#include "can_utility.h"
#include "string.h"

HAL_StatusTypeDef Flash_Erase_NewSlot()
{
  // Prepare Flash erase structure
  FLASH_EraseInitTypeDef erase;

  erase.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase.VoltageRange = VOLTAGE_RANGE_3;

  // we want to erase the slot of the new app
#if DISABLE_TWO_SLOT_SOLUTION
  erase.Sector = USER_APP_SLOT_1_START;
  erase.NbSectors = USER_APP_SLOT_1_SECTOR_CNT;
#else
  if (old_app_slot == USER_APP_SLOT_1) {
    erase.Sector = USER_APP_SLOT_2_START;
    erase.NbSectors = USER_APP_SLOT_2_SECTOR_CNT;
  } else {
    // old_app_slot is either none or 2
    erase.Sector = USER_APP_SLOT_1_START;
    erase.NbSectors = USER_APP_SLOT_1_SECTOR_CNT;
  }
#endif

  // Disable interrupts for Flash critical section
  __disable_irq();

  // Unlock Flash memory
  HAL_FLASH_Unlock();

  // Reset Flash flags
  FLASH_CLEAR_FLAGS();

  uint32_t error;

  // Erase user application slots
  HAL_StatusTypeDef result = HAL_FLASHEx_Erase(&erase, &error);

  // Indication for fallback procedures
  slot_erased = 1;

  // Lock Flash again after completing operation
  HAL_FLASH_Lock();

  // Enable interrupts after Flash critical section
  __enable_irq();

  return result;
}

HAL_StatusTypeDef Flash_Write_CANRxBuffer(uint32_t offset)
{
  HAL_StatusTypeDef result;

#ifdef TWO_SLOT_SOLUTION
  uint32_t base_address = USER_APP_SLOT_1_ADDR;
#else
  // We want to write to the slot of the new app
  uint32_t base_address = old_app_slot == USER_APP_SLOT_1 ?
                            USER_APP_SLOT_2_ADDR : USER_APP_SLOT_1_ADDR;
#endif
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

#ifndef DISABLE_TWO_SLOT_SOLUTION

enum UserApplicationSlot Read_Old_App_Slot() {
  __IO uint32_t read_byte = *(__IO uint32_t *)OLD_APP_SLOT_INDICATOR_ADDR;
  uint32_t indicator = (uint32_t)read_byte;
  // clamp it to the possible values just in case
  if (indicator == USER_APP_SLOT_1 || indicator == USER_APP_SLOT_2 ||
      indicator == USER_APP_SLOT_NONE) {
    return (enum UserApplicationSlot)indicator;
  } else {
    return USER_APP_SLOT_NONE;
  }
}

HAL_StatusTypeDef Write_New_App_Slot() {
  __disable_irq();
  HAL_FLASH_Unlock();

  FLASH_CLEAR_FLAGS();
  FLASH_WaitForLastOperation(50000);

  HAL_StatusTypeDef result = HAL_FLASH_Program(
    FLASH_TYPEPROGRAM_WORD, OLD_APP_SLOT_INDICATOR_ADDR, (uint32_t)old_app_slot
  );

  HAL_FLASH_Lock();
  __enable_irq();

  return result;
}

#endif
