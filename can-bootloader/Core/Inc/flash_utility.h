#ifndef __FLASH_UTILITY_H
#define __FLASH_UTILITY_H

#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

// User application slot
enum UserApplicationSlot {
    USER_APP_SLOT_NONE = 0x00,
    USER_APP_SLOT_1 = 0x55,
    USER_APP_SLOT_2 = 0xAA
};

// Address where user application slot 1 starts
// (beginning of FLASH_SECTOR_1)
#define USER_APP_SLOT_1_ADDR 0x08008000U

// Flash memory sector where user application slot 1 starts
#define USER_APP_SLOT_1_START FLASH_SECTOR_1

// Set this to make the bootloader utilize the whole memory
// for the application
#ifdef DISABLE_TWO_SLOT_SOLUTION

// Number of sectors reserved for user application slot 1
#define USER_APP_SLOT_1_SECTOR_CNT 7U

#else

// Address where an indicator of the last loaded application
// resides. Should be the last byte of flash memory allocated
// to the bootloader.
#define OLD_APP_SLOT_INDICATOR_ADDR ((USER_APP_SLOT_1_ADDR) - 1U)

// Address where user application slot 2 starts
// (beginning of FLASH_SECTOR_6)
#define USER_APP_SLOT_2_ADDR 0x08080000U

// Flash memory sector where user application slot 2 starts
#define USER_APP_SLOT_2_START FLASH_SECTOR_6

// Number of sectors reserved for user application slot 1
#define USER_APP_SLOT_1_SECTOR_CNT 5U

// Number of sectors reserved for user application slot 2
#define USER_APP_SLOT_2_SECTOR_CNT 2U

#endif // DISABLE_TWO_SLOT_SOLUTION

// Macro for clearing Flash flags
#define FLASH_CLEAR_FLAGS() \
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | \
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | \
                           FLASH_FLAG_PGPERR | FLASH_FLAG_ERSERR)

// Erase Flash sectors for target slot
HAL_StatusTypeDef Flash_Erase_NewSlot();

// Write content of CAN receive buffer to target slot
// moved by offset
HAL_StatusTypeDef Flash_Write_CANRxBuffer(uint32_t offset);


#ifndef DISABLE_TWO_SLOT_SOLUTION
enum UserApplicationSlot Read_Old_App_Slot();
HAL_StatusTypeDef Write_New_App_Slot();
#endif

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_UTILITY_H */
