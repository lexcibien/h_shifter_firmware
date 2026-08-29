# Firmware para H-shifter (RP2040 Zero)

cmake_minimum_required(VERSION 3.35)

set(PICO_BOARD waveshare_rp2040_zero CACHE STRING "Board type")

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(
  H_SHIFTER_JAPATECH
  DESCRIPTION "Firmware para o câmbio com RP2040 Zero, lê 8 entradas do câmbio e apresenta 12 botões virtuais ao PC como Joystick"
  LANGUAGES C CXX ASM
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)
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
  -Wno-format # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
  -Wno-unused-function # we have some for the docs that aren't called
  -fstack-usage
  -Os
)

if(TARGET tinyusb_device)
  set(FAMILY rp2040)
  set(BOARD pico_sdk)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  # Some examples use this, and we need to set this here due to a bug in the TinyUSB CMake config
  set(TOP ${PICO_TINYUSB_PATH})

  set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
  set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
  file(GLOB_RECURSE SRC_FILES ${SRC_DIR}/*.c ${SRC_DIR}/*.cpp ${LIB_DIR}/*.cpp)
  add_executable(H_SHIFTER_JAPATECH ${SRC_FILES})
  target_include_directories(H_SHIFTER_JAPATECH PRIVATE
    ${SRC_DIR}
  )

  target_compile_options(H_SHIFTER_JAPATECH PRIVATE ${H_SHIFTER_JAPATECH_COMPILE_OPTIONS})
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(H_SHIFTER_JAPATECH PRIVATE -Wno-maybe-uninitialized)
  endif()

  target_compile_definitions(H_SHIFTER_JAPATECH PUBLIC
    USB_MANUFACTURER="Japatech Projects"
    USB_PRODUCT="Shifter Truck Arduino"
    USBD_MAX_POWER_MA=250
    TINYUSB_NEED_POLLING_TASK
    DEBUG
  )

  target_link_libraries(H_SHIFTER_JAPATECH PUBLIC
    pico_stdlib
    pico_unique_id
    tinyusb_device
    tinyusb_board
    hardware_pwm
    hardware_clocks
    hardware_gpio
    hardware_pll
    hardware_adc
  )

  pico_enable_stdio_usb(H_SHIFTER_JAPATECH 1)
  pico_add_extra_outputs(H_SHIFTER_JAPATECH)

  add_custom_command(TARGET H_SHIFTER_JAPATECH POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
  )

  add_custom_target(size_report
    COMMAND ${CMAKE_COMMAND} -E echo "=== Memory Usage Report ==="
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/scripts/memory_size.py
  )

  add_custom_command(TARGET H_SHIFTER_JAPATECH POST_BUILD COMMAND ${CMAKE_COMMAND} --build . --target size_report)

  set(FIRMWARE_NAME ${PROJECT_NAME})
  add_custom_target(flash
    COMMAND picotool load -f -x
    ${CMAKE_BINARY_DIR}/${FIRMWARE_NAME}.bin -o 0x10000000
    DEPENDS ${FIRMWARE_NAME}
  )

else()
  message("Skipping TinyUSB device examples as TinyUSB is unavailable")
endif()
