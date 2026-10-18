# ************** #
#  BASIC CONFIG  #
# ************** #

cmake_minimum_required(VERSION 3.20.0)

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild)

# ******************** #
#  COMPILE BOOTLOADER  #
# ******************** #

include(ExternalProject)

# Bootloader source dir (STM32Cube project)
set(BOOTLOADER_DIR
    ${CMAKE_CURRENT_LIST_DIR}/../../can-bootloader
)

# Bootloader destination build
set(BOOTLOADER_BUILD_DIR
    ${CMAKE_BINARY_DIR}/can-bootloader
)

# Bootloader GCC toolchain
set(STM32_GCC_DIR
    "$ENV{HOME}/.local/share/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin"
)

# Create bootloader environment
set(BOOTLOADER_ENV
    ${CMAKE_COMMAND} -E env
    "PATH=${STM32_GCC_DIR}:$ENV{PATH}"
)

# Bootloade binary, result of compilation
set(BOOTLOADER_ELF
    "${BOOTLOADER_BUILD_DIR}/can-bootloader.elf"
)

# Actual bootloader build command
ExternalProject_Add(bootloader
    SOURCE_DIR ${BOOTLOADER_DIR}
    BINARY_DIR ${BOOTLOADER_BUILD_DIR}

    CONFIGURE_COMMAND
        ${BOOTLOADER_ENV}
        ${CMAKE_COMMAND}
        -DCMAKE_BUILD_TYPE=Debug
        -DCMAKE_TOOLCHAIN_FILE=${BOOTLOADER_DIR}/cmake/gcc-arm-none-eabi.cmake
        -G Ninja
        -S ${BOOTLOADER_DIR}
        -B ${BOOTLOADER_BUILD_DIR}

    BUILD_COMMAND
        ${BOOTLOADER_ENV}
        ${CMAKE_COMMAND}
        --build ${BOOTLOADER_BUILD_DIR}

    INSTALL_COMMAND ""

    BUILD_BYPRODUCTS
        "${BOOTLOADER_ELF}"

    BUILD_ALWAYS TRUE
)

# ****************** #
#  MERGE 2 BINARIES  #
# ****************** #

# Programs used for merging 2 binaries, install if missing
find_program(ARM_OBJCOPY
    arm-none-eabi-objcopy
    HINTS "${STM32_GCC_DIR}"
    REQUIRED
)
find_program(SREC_CAT
    srec_cat
    REQUIRED
)


# User application binary
set(APP_ELF
    "${CMAKE_BINARY_DIR}/user-application/zephyr/zephyr.elf"
)


# Intermediate binaries
set(BOOTLOADER_HEX
    "${CMAKE_BINARY_DIR}/bootloader.hex"
)
set(APP_HEX
    "${CMAKE_BINARY_DIR}/application.hex"
)


# Final binary, this is what you flash
set(COMBINED_HEX
    "${CMAKE_BINARY_DIR}/merged.hex"
)


# Actual merging that produces ${COMBINED_HEX}
add_custom_command(
    OUTPUT "${COMBINED_HEX}"

    COMMAND "${ARM_OBJCOPY}"
        -O ihex
        "${BOOTLOADER_ELF}"
        "${BOOTLOADER_HEX}"

    COMMAND "${ARM_OBJCOPY}"
        -O ihex
        "${APP_ELF}"
        "${APP_HEX}"

    COMMAND "${SREC_CAT}"
        "${BOOTLOADER_HEX}" -Intel
        "${APP_HEX}" -Intel
        -o "${COMBINED_HEX}" -Intel

    DEPENDS
        "${BOOTLOADER_ELF}"
        "${APP_ELF}"

    COMMENT "Merging bootloader and application HEX files"

    VERBATIM
)

add_custom_target(merge_firmware ALL
    DEPENDS "${COMBINED_HEX}"
)

add_dependencies(merge_firmware bootloader)
