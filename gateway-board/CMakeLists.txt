cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(gateway_board)

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

set(GENERATED_C ${GENERATED_DIR}/dummy.c)
set(GENERATED_H ${GENERATED_DIR}/dummy.h)

add_custom_command(
    OUTPUT
        ${GENERATED_C}
        ${GENERATED_H}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND python3 -m cantools generate_c_source
            -o ${GENERATED_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../dummy.dbc
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/../dummy.dbc
    COMMENT "Generating CAN source files with cantools"
    VERBATIM
)

add_custom_target(generate_can_sources
    DEPENDS
        ${GENERATED_C}
        ${GENERATED_H}
)

file(GLOB APP_SOURCES "src/*.c")

target_sources(app PRIVATE
    ${APP_SOURCES}
    ${GENERATED_C}
)

target_include_directories(app PRIVATE
    inc/
    ${GENERATED_DIR}
)

add_dependencies(app generate_can_sources)
