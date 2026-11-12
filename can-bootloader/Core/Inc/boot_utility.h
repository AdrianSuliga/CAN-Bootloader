#ifndef __BOOT_UTILITY_H
#define __BOOT_UTILITY_H

#include "stm32f7xx_hal.h"
#include "flash_utility.h"

#ifdef __cplusplus
extern "C" {
#endif

// Jump to program stored at user application slot
void Boot_Start_UserApplication();

// Try to recover the system after a critical error
void Boot_Recover();

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_UTILITY_H */
