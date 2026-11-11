cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(user_application)

if(SB_CONFIG_SYSBUILD)
    zephyr_get(CTRL_FRAME_ID SYSBUILD GLOBAL)
endif()

if(NOT "${CTRL_FRAME_ID}" MATCHES "^-?[0-9]+$")
    message(FATAL_ERROR "CTRL_FRAME_ID must be an integer, got '${CTRL_FRAME_ID}'")
endif()

add_compile_definitions(SYSBUILD_CTRL_FRAME_ID=${CTRL_FRAME_ID})

target_sources(app PRIVATE src/main.c)
