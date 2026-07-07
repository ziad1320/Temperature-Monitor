cmake_minimum_required(VERSION 3.30)
include(cmake/ArmToolchain.cmake)

project(stm32-test C)

set(CMAKE_C_STANDARD                11)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)
enable_language(ASM)

set(PROJECT_PATH                         ${CMAKE_CURRENT_SOURCE_DIR})
include(cmake/Device.cmake)
include(cmake/SourceList.cmake)
include(cmake/IncludeList.cmake)

set(LINKER_SCRIPTS_BASE_DIR              ${PROJECT_PATH}/STM32-base/linker/)
set(STARTUP_BASE_DIR                     ${PROJECT_PATH}/STM32-base/startup/)

set(EXECUTABLE                           ${CMAKE_PROJECT_NAME})

# === NEW: Automatically find and include all necessary HAL source files ===
file(GLOB HAL_SOURCES "STM32-base-STM32Cube/HAL/STM32F4xx/src/*.c")
# Exclude template files to prevent multiple definition errors
list(FILTER HAL_SOURCES EXCLUDE REGEX ".*_template\\.c$")

# Executable files
add_executable(${EXECUTABLE}
        ${SOURCE_LIST}
        src/onewire.c
        ${HAL_SOURCES}
        ${STARTUP_BASE_DIR}/${SERIES_FOLDER}/${MAPPED_DEVICE}.s
)

# Include directories
target_include_directories(${EXECUTABLE} PUBLIC
        ${INCLUDE_LIST}
        include
        STM32-base-STM32Cube/HAL/STM32F4xx/inc
        STM32-base-STM32Cube/HAL/STM32F4xx/inc/Legacy
        STM32-base-STM32Cube/CMSIS/STM32F4xx/inc
        STM32-base-STM32Cube/CMSIS/ARM/inc
)

# Project symbols
target_compile_definitions(${EXECUTABLE} PUBLIC ${COMPILER_DEFINE})

# Compiler options
target_compile_options(${EXECUTABLE} PUBLIC
        ${CPU_PARAMETERS}
        -Wall
        -Wpedantic
        -Wno-unused-parameter
)

# Linker options
target_link_options(${EXECUTABLE} PRIVATE
        ${CPU_PARAMETERS}
        -L${LINKER_SCRIPTS_BASE_DIR}
        -T${LINKER_SCRIPTS_BASE_DIR}/${SERIES_FOLDER}/${DEVICE}.ld
        -Xlinker --gc-sections
        -Wl,-Map=${CMAKE_PROJECT_NAME}.map
        --specs=nano.specs
)

# Execute post-build to print size
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${EXECUTABLE}>
)
# Convert output to hex and binary
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXECUTABLE}> ${EXECUTABLE}.hex
)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}> ${EXECUTABLE}.bin
)