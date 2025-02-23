cmake_minimum_required(VERSION 3.10)

# Project name
project(smartfusion_bootloader C ASM)

SET(BUILD_MODE Debug)
# Define the CPU and compiler flags
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linkerscript.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${LINKER_SCRIPT}")

# Set cpu to cortex-m3
set(CPU_FLAGS "-mcpu=cortex-m3")
# Set compiler flags
set(CMAKE_BUILD_TYPE Debug)
add_compile_options(${CPU_FLAGS} ${COMMON_FLAGS})
SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
# SET(CMAKE_CXX_FLAGS "-DMICROSEMI_STDIO_THRU_MMUART0 -DMICROSEMI_STDIO_BAUD_RATE=921600")

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
    ${CMAKE_SOURCE_DIR}/src/ring-buffer.c
    ${CMAKE_SOURCE_DIR}/src/bootloader.c
    ${CMAKE_SOURCE_DIR}/src/simple-sw-timer.c
    ${CMAKE_SOURCE_DIR}/src/sys-time.c
    ${CMAKE_SOURCE_DIR}/src/comms.c
    ${CMAKE_SOURCE_DIR}/src/uart.c
    ${CMAKE_SOURCE_DIR}/src/led.c
    ${CMAKE_SOURCE_DIR}/src/main.c
)

file(GLOB_RECURSE ASMSOURCES
    ${FIRMWARE_DIR}/CMSIS/startup_gcc/*.S
    ${FIRMWARE_DIR}/hal/CortexM3/*.s
    ${FIRMWARE_DIR}/CMSIS/**/*.s
)

set(TARGET_NAME ${PROJECT_NAME}.elf)
add_executable(${TARGET_NAME} ${SOURCES} ${ASMSOURCES})

# Post-build steps
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary ${TARGET_NAME} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND python3 ${CMAKE_SOURCE_DIR}/pad-bootloader.py ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMENT "Generating BIN and padding bootloader"
)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD 
    COMMAND arm-none-eabi-objcopy -O ihex ${TARGET_NAME} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND arm-none-eabi-size --format=berkeley ${TARGET_NAME}
    COMMENT "Generating HEX and size information"
)
