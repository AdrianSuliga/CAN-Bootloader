#ifndef __BOOT_UTILITY_H
#define __BOOT_UTILITY_H

#include "stm32f7xx_hal.h"
#include "flash_utility.h"

#ifdef __cplusplus
extern "C" {
#endif

// Declared in boot_utility.h, defined in boot_utility.c
// System state, keeps track of user application slot cycle
extern enum UserApplicationSlot target_slot;
extern enum UserApplicationSlot recovery_slot;

// Jump to user application stored at target slot
void Boot_Start_NewUserApplication();

// Jump to user application stored at recovery slot
void Boot_Recover_OldUserApplication();

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_UTILITY_H */
