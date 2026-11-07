# Firmware para H-shifter (RP2040 Zero)

cmake_minimum_required(VERSION 3.35)

set(PICO_BOARD waveshare_rp2040_zero CACHE STRING "Board type")
option(H_SHIFTER_LOG "Enable operational logs over USB CDC" OFF)
set(SHIFTER_PROJECT_VERSION "SHIFTER_PROJECT_V1" CACHE STRING "H-shifter hardware/logic version")
set_property(CACHE SHIFTER_PROJECT_VERSION PROPERTY STRINGS SHIFTER_PROJECT_V1 SHIFTER_PROJECT_V2)

if(SHIFTER_PROJECT_VERSION STREQUAL "SHIFTER_PROJECT_V1")
  set(H_SHIFTER_USB_PRODUCT "DAZ Shifter V1")
elseif(SHIFTER_PROJECT_VERSION STREQUAL "SHIFTER_PROJECT_V2")
  set(H_SHIFTER_USB_PRODUCT "DAZ Shifter V2")
else()
  message(FATAL_ERROR
    "Invalid SHIFTER_PROJECT_VERSION '${SHIFTER_PROJECT_VERSION}'. "
    "Select SHIFTER_PROJECT_V1 or SHIFTER_PROJECT_V2."
  )
endif()

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(
  H_SHIFTER_JAPATECH
  DESCRIPTION "Firmware para o câmbio com RP2040 Zero, com projetos V1 de 12 botões e V2 de 13 botões virtuais."
  LANGUAGES C CXX ASM
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(PICO_SDK_VERSION_STRING VERSION_LESS "2.3.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

# Compiler flags for the firmware target
set(H_SHIFTER_JAPATECH_COMPILE_OPTIONS
  -Wall
  -Wno-unused-function # we have some for the docs that aren't called
  -fstack-usage
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.c ${SRC_DIR}/*.cpp ${LIB_DIR}/*.cpp)
add_executable(H_SHIFTER_JAPATECH ${SRC_FILES})
target_include_directories(H_SHIFTER_JAPATECH PRIVATE
  ${SRC_DIR}
)

target_compile_options(H_SHIFTER_JAPATECH PRIVATE ${H_SHIFTER_JAPATECH_COMPILE_OPTIONS})
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_compile_options(H_SHIFTER_JAPATECH PRIVATE -Wno-maybe-uninitialized)
endif()

target_compile_definitions(H_SHIFTER_JAPATECH PRIVATE
  SHIFTER_PROJECT_VERSION=${SHIFTER_PROJECT_VERSION}
  USB_MANUFACTURER="Japatech Projects"
  USB_PRODUCT="${H_SHIFTER_USB_PRODUCT}"
  H_SHIFTER_LOG=$<BOOL:${H_SHIFTER_LOG}>
  H_SHIFTER_ENABLE_CDC=$<BOOL:${H_SHIFTER_LOG}>
  TINYUSB_NEED_POLLING_TASK
)

target_link_libraries(H_SHIFTER_JAPATECH PRIVATE
  pico_stdlib
  tinyusb_device
  tinyusb_board
  hardware_adc
)

pico_enable_stdio_usb(H_SHIFTER_JAPATECH ${H_SHIFTER_LOG})
pico_enable_stdio_uart(H_SHIFTER_JAPATECH 0)
pico_add_extra_outputs(H_SHIFTER_JAPATECH)

add_custom_command(TARGET H_SHIFTER_JAPATECH POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_BINARY_DIR}/compile_commands.json
  ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
)

add_custom_command(TARGET H_SHIFTER_JAPATECH POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "=== Memory Usage Report ==="
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/memory_size.py
    --file $<TARGET_FILE:H_SHIFTER_JAPATECH>
    --board ${PICO_BOARD}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM
)

set(FIRMWARE_NAME ${PROJECT_NAME})
add_custom_target(flash
  COMMAND picotool load -f -x
  ${CMAKE_BINARY_DIR}/${FIRMWARE_NAME}.bin -o 0x10000000
  DEPENDS ${FIRMWARE_NAME}
)
