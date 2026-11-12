#include "flash_utility.h"
#include "boot_utility.h"
#include "can_utility.h"
#include "string.h"

HAL_StatusTypeDef Flash_Erase_TargetSlot(enum UserApplicationSlot slot)
{
  uint32_t sector_idx, sector_cnt;
  switch (slot) {
    case USER_APP_SLOT:
      sector_idx = USER_APP_SLOT_START;
      sector_cnt = USER_APP_SLOT_SECTOR_CNT;
      slot_erased = 1;
      break;
#ifdef TWO_SLOT_SOLUTION
    case SWAP_SLOT:
      sector_idx = SWAP_SLOT_START;
      sector_cnt = SWAP_SLOT_SECTOR_CNT;
      break;
#endif
    default:
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
  erase.Sector = sector_idx;
  erase.NbSectors = sector_cnt;

  // Erase user application slots
  HAL_StatusTypeDef result = HAL_FLASHEx_Erase(&erase, &error);

  // Lock Flash again after completing operation
  HAL_FLASH_Lock();

  // Enable interrupts after Flash critical section
  __enable_irq();

  return result;
}

HAL_StatusTypeDef Flash_Write(
  enum UserApplicationSlot target_slot,
  uint32_t offset,
  uint8_t *source,
  uint32_t len
) {
  // Determine address of the target slot
  uint32_t base_address;
  switch (target_slot) {
    case USER_APP_SLOT:
      base_address = USER_APP_SLOT_ADDR;
      break;
#ifdef TWO_SLOT_SOLUTION
    case SWAP_SLOT:
      base_address = SWAP_SLOT_ADDR;
      break;
#endif
    default:
      return HAL_ERROR;
  }

  HAL_StatusTypeDef result;
  uint32_t write_address = base_address + offset;

  // Disable interrupts for Flash critical section
  __disable_irq();
  // Unlock Flash memory
  HAL_FLASH_Unlock();

  // Loop through buffer and save its content to user application slot
  for (int i = 0; i < len; i += sizeof(uint32_t)) {
    // Flash memory is arranged in 32-bit words
    uint32_t word;
    memcpy(&word, (void*)(source + i), sizeof(uint32_t));

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

#ifdef TWO_SLOT_SOLUTION
HAL_StatusTypeDef Flash_Move_Swap() {
  HAL_StatusTypeDef res;

  // Prepare the flash memory of user application
  res = Flash_Erase_TargetSlot(USER_APP_SLOT);
  if (res != HAL_OK) {
    return res;
  }

  // Move the data from swap space to user application space
  uint32_t app_len = flash_new_app_offset;
  res = Flash_Write(USER_APP_SLOT, 0, (uint8_t*)SWAP_SLOT_ADDR, app_len);
  if (res != HAL_OK) {
    return res;
  }

  // Erase the swap space. An error here would not be critical, so we ignore it
  Flash_Erase_TargetSlot(SWAP_SLOT);

  return HAL_OK;
}
#endif
