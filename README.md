# CAN-Bootloader
Bootloader supporting flashing new application via CAN

# Structure
- ```can-bootloader``` - bootloader for STM32F7 boards, it is supposed to receive new app via CAN and write it to FLASH.
- ```user-application``` - test user application that is supposed to be written to FLASH by ```can-bootloader```.
- ```gateway-board``` - Zephyr RTOS application meant to receive ```user-application``` wirelessly and send it via CAN to STM32F7 board running ```can-bootloader```.
