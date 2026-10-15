#include "boot_utility.h"
#include "can_utility.h"
#include "flash_utility.h"

// Initiate invalid state, recover actual state in main.c
enum UserApplicationSlot old_app_slot = USER_APP_SLOT_NONE;

void Boot_Start_Application(enum UserApplicationSlot slot)
{
  // Jump to the slot pointed by the argument, or slot 1 by default
#ifdef DISABLE_TWO_SLOT_SOLUTION
  uint32_t target = USER_APP_SLOT_1_ADDR;
#else
  uint32_t target = slot == USER_APP_SLOT_2 ?
                      USER_APP_SLOT_2_ADDR : USER_APP_SLOT_1_ADDR;
#endif

  // Disable interrupts for critical section
  __disable_irq();

  // Deinitialize all HAL peripherals and stop SysTick
  HAL_DeInit();

  // Reset SysTick
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;

  // Set vector table to the one used by user application
  SCB->VTOR = target;

  // Set stack pointer to the one used by user application
  volatile uint32_t newStackPtr;
  newStackPtr = *(volatile uint32_t*)(target);

  __set_MSP(newStackPtr);

  // Prepare „fake” function to jump to user application
  volatile uint32_t codeAddress;
  codeAddress = *(volatile uint32_t*)(target + 4);

  void (*app_entry)(void) = (void*)codeAddress;

  // Enable interrupts before the jump
  __enable_irq();

  // Barrier for memory instructions right before the jump
  __DSB();

  // User application starts here
  app_entry();
}

void Boot_Recover_OldUserApplication()
{
#ifdef DISABLE_TWO_SLOT_SOLUTION
  if (slot_erased) {
    // There is no fallback option.
    // Bootloader hangs indefinitely. 
    while (1) {
      HAL_Delay(1000);
    }
  } else {
    Boot_Start_NewUserApplication(old_app_slot);
  }
#else
  if (old_app_slot == USER_APP_SLOT_NONE) {
    while (1) {
      // There is no fallback option.
      // Bootloader hangs indefinitely.
      HAL_Delay(1000);
    }
  } else {
    Boot_Start_Application(old_app_slot);
  }
#endif
}
