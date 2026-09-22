#ifndef __FLASH_UTILITY_H
#define __FLASH_UTILITY_H

#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

// User application slot
enum UserApplicationSlot {
    USER_APP_SLOT_INVALID = -1,
    USER_APP_SLOT_1,
    USER_APP_SLOT_2
};

// Address where user application slot 1 starts
// (beginning of FLASH_SECTOR_1)
#define USER_APP_SLOT_1_ADDR 0x08008000U

// Address where user application slot 2 starts
// (beginning of FLASH_SECTOR_6)
#define USER_APP_SLOT_2_ADDR 0x08080000U

// Flash memory sector where user application slot 1 starts
#define USER_APP_SLOT_1_START FLASH_SECTOR_1

// Flash memory sector where user application slot 2 starts
#define USER_APP_SLOT_2_START FLASH_SECTOR_6

// Number of sectors reserved for user application slot 1
#define USER_APP_SLOT_1_SECTOR_CNT 5U

// Number of sectors reserved for user application slot 2
#define USER_APP_SLOT_2_SECTOR_CNT 2U

// Macro for clearing Flash flags
#define FLASH_CLEAR_FLAGS() \
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | \
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | \
                           FLASH_FLAG_PGPERR | FLASH_FLAG_ERSERR)

// Erase Flash sectors for target slot
HAL_StatusTypeDef Flash_Erase_TargetSlot();

// Write content of CAN receive buffer to target slot
// moved by offset
HAL_StatusTypeDef Flash_Write_CANRxBuffer(uint32_t offset);

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_UTILITY_H */
