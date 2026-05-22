#include "boot_utility.h"
#include "flash_utility.h"

void jump_to_app(enum UserApplicationSlot slot)
{
  // Verify caller wants to jump to valid slot
  if (slot != USER_APP_SLOT_1 && slot != USER_APP_SLOT_2) {
    return;
  }

  uint32_t target = slot == USER_APP_SLOT_1 ? USER_APP_SLOT_1_ADDR : USER_APP_SLOT_2_ADDR;

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
