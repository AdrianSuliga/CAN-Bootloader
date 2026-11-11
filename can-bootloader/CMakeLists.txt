cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME can-bootloader)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/flash_utility.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/can_utility.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/boot_utility.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/crc_utility.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

option(TWO_SLOT_SOLUTION "Enable two slot solution" ON)

if (TWO_SLOT_SOLUTION)
    add_compile_definitions(TWO_SLOT_SOLUTION)
endif()

set(CTRL_FRAME_ID "" CACHE STRING "ID of control frame")
set(FIRMWARE_FRAME_ID "" CACHE STRING "ID of firmware frame")

if ("${CTRL_FRAME_ID}" STREQUAL "")
    message(FATAL_ERROR "[Bootloader] CTRL_FRAME_ID must be provided via -DCTRL_FRAME_ID=<integer>")
endif()

if ("${FIRMWARE_FRAME_ID}" STREQUAL "")
    message(FATAL_ERROR "[Bootloader] FIRMWARE_FRAME_ID must be provided via -DFIRMWARE_FRAME_ID=<integer>")
endif()

if (NOT "${CTRL_FRAME_ID}" MATCHES "^-?[0-9]+$")
    message(FATAL_ERROR "[Bootloader] CTRL_FRAME_ID must be an integer, got '${CTRL_FRAME_ID}'")
endif()

if (NOT "${FIRMWARE_FRAME_ID}" MATCHES "^-?[0-9]+$")
    message(FATAL_ERROR "[Bootloader] FIRMWARE_FRAME_ID must be an integer, got '${FIRMWARE_FRAME_ID}'")
endif()

add_compile_definitions(SYSBUILD_CAN_CTRL_FRAME_ID=${CTRL_FRAME_ID})
add_compile_definitions(SYSBUILD_CAN_FIRMWARE_FRAME_ID=${FIRMWARE_FRAME_ID})
