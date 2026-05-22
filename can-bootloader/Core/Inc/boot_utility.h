#ifndef __BOOT_UTILITY_H
#define __BOOT_UTILITY_H

#include "stm32f7xx_hal.h"
#include "flash_utility.h"

#ifdef __cplusplus
extern "C" {
#endif

// Jump to user application stored at given slot
void jump_to_app(enum UserApplicationSlot slot);

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_UTILITY_H */
