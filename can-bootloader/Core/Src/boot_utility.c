#include "boot_utility.h"
#include "can_utility.h"
#include "flash_utility.h"

void Boot_Start_UserApplication()
{
  // Disable interrupts for critical section
  __disable_irq();
  // Deinitialize all HAL peripherals and stop SysTick
  HAL_DeInit();

  // Reset SysTick
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;

  // Set vector table to the one used by user application
  SCB->VTOR = USER_APP_SLOT_ADDR;

  // Set stack pointer to the one used by user application
  volatile uint32_t newStackPtr;
  newStackPtr = *(volatile uint32_t*)(USER_APP_SLOT_ADDR);

  __set_MSP(newStackPtr);

  // Prepare „fake” function to jump to user application
  volatile uint32_t codeAddress;
  codeAddress = *(volatile uint32_t*)(USER_APP_SLOT_ADDR + 4);

  void (*app_entry)(void) = (void*)codeAddress;

  // Enable interrupts before the jump
  __enable_irq();

  // Barrier for memory instructions right before the jump
  __DSB();

  // User application starts here
  app_entry();
}

void Boot_Recover()
{
  if (slot_erased) {
    // Flash was erased, there is no fallback option.
    // Bootloader hangs indefinitely. 
    while (1) {
      HAL_Delay(1000);
    }
  } else {
    Boot_Start_UserApplication();
  }
}
