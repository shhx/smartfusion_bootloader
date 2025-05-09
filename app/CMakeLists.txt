cmake_minimum_required(VERSION 3.10)

# Project name
project(smartfusion_app C ASM)

SET(BUILD_MODE Debug)
# Define the CPU and compiler flags
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linkerscript.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${LINKER_SCRIPT}")

# Set cpu to cortex-m3
set(CPU_FLAGS "-mcpu=cortex-m3")
# Set compiler flags
add_compile_options(${CPU_FLAGS} ${COMMON_FLAGS})

SET(FIRMWARE_DIR ${CMAKE_SOURCE_DIR}/../firmware/)
# Include directories
include_directories(
    ${FIRMWARE_DIR}
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/../ARM_CMSIS/CMSIS/Include
)

# Source files
file(GLOB_RECURSE SOURCES
    ${FIRMWARE_DIR}/hal/CortexM3/*.c
    ${FIRMWARE_DIR}/CMSIS/startup_gcc/*.c
    ${FIRMWARE_DIR}/drivers/**/*.c
    ${FIRMWARE_DIR}/drivers_config/**/*.c
    ${FIRMWARE_DIR}/*.c
    ${CMAKE_SOURCE_DIR}/src/bootloader.S
    ${CMAKE_SOURCE_DIR}/src/main.c
)

file(GLOB_RECURSE ASMSOURCES
    ${FIRMWARE_DIR}/CMSIS/startup_gcc/*.S
    ${FIRMWARE_DIR}/hal/CortexM3/*.s
    ${FIRMWARE_DIR}/CMSIS/**/*.s
)

set(TARGET_NAME ${PROJECT_NAME}.elf)
add_executable(${TARGET_NAME} ${SOURCES} ${ASMSOURCES})
set_target_properties(${TARGET_NAME} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# Post-build steps
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary ${TARGET_NAME} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMENT "Generating BIN"
)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD 
    COMMAND arm-none-eabi-objcopy -O ihex ${TARGET_NAME} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND arm-none-eabi-size --format=berkeley ${TARGET_NAME}
    COMMENT "Generating HEX and size information"
)
