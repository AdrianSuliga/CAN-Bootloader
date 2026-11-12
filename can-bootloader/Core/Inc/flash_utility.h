#ifndef __FLASH_UTILITY_H
#define __FLASH_UTILITY_H

#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

enum UserApplicationSlot {
    USER_APP_SLOT,
#ifdef TWO_SLOT_SOLUTION
    SWAP_SLOT
#endif
};

// Address where user application slot starts (beginning of FLASH_SECTOR_1)
#define USER_APP_SLOT_ADDR 0x08008000U

// Flash memory sector where user application slot starts
#define USER_APP_SLOT_START FLASH_SECTOR_1

#ifdef TWO_SLOT_SOLUTION

// Number of sectors reserved for user application slot
#define USER_APP_SLOT_SECTOR_CNT 5U

// Address where swap slot starts (beginning of FLASH_SECTOR_6)
#define SWAP_SLOT_ADDR 0x08080000U

// Flash memory sector where swap slot starts
#define SWAP_SLOT_START FLASH_SECTOR_6

// Number of sectors reserved for swap slot
#define SWAP_SLOT_SECTOR_CNT 2U

#else

#define USER_APP_SLOT_SECTOR_CNT 7U

#endif // TWO_SLOT_SOLUTION

// Macro for clearing Flash flags
#define FLASH_CLEAR_FLAGS() \
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | \
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | \
                           FLASH_FLAG_PGPERR | FLASH_FLAG_ERSERR)

// Erase Flash sectors for target slot
HAL_StatusTypeDef Flash_Erase_TargetSlot(enum UserApplicationSlot slot);

// Write content of source to slot moved by offset
HAL_StatusTypeDef Flash_Write(
  enum UserApplicationSlot target_slot,
  uint32_t offset,
  uint8_t *source,
  uint32_t len
);

#ifdef TWO_SLOT_SOLUTION
// Move the contents of swap space to user application space
HAL_StatusTypeDef Flash_Move_Swap();
#endif

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_UTILITY_H */
