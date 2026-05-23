cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(
  H_SHIFTER_JAPATECH
  VERSION 1.0.0
  DESCRIPTION "Firmware para o câmbio com Arduino Leonardo, lê 4 entradas do câmbio e apresenta 6 botões virtuais ao PC como gamepad"
  LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)

if(PICO_SDK_VERSION_STRING VERSION_LESS "2.2.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.2.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# If you want debug output from USB (pass -DPICO_STDIO_USB=1) this ensures you don't lose any debug output while USB is set up
if(NOT DEFINED PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS)
  set(PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS 3000)
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
  -Wno-format # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
  -Wno-unused-function # we have some for the docs that aren't called
)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()

if(TARGET tinyusb_device)
  set(FAMILY rp2350)
  set(BOARD pico_sdk)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  # Some examples use this, and we need to set this here due to a bug in the TinyUSB CMake config
  set(TOP ${PICO_TINYUSB_PATH})

  file(GLOB_RECURSE PROJECT_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.c
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/*.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/*.cpp
  )

  add_executable(H_SHIFTER_JAPATECH)

  # target_sources(H_SHIFTER_JAPATECH PUBLIC
  #   ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
  #   ${CMAKE_CURRENT_LIST_DIR}/src/USB.cpp
  #   ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c
  # )
  target_sources(H_SHIFTER_JAPATECH PUBLIC ${PROJECT_SOURCES})

  # Make sure TinyUSB can find tusb_config.h
  target_include_directories(H_SHIFTER_JAPATECH PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/lib
    ${CMAKE_CURRENT_LIST_DIR}/boards
    ${CMAKE_CURRENT_LIST_DIR}
  )

  # In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
  # for TinyUSB device support and tinyusb_board for the additional board support library used by the example
  target_link_libraries(H_SHIFTER_JAPATECH PUBLIC pico_stdlib pico_unique_id tinyusb_device tinyusb_board hardware_pwm hardware_clocks hardware_gpio hardware_pll hardware_adc)

  # Provide Arduino-style board macro to sources (so headers using #if ARDUINO_RASPBERRY_PI_PICO_2 work)
  target_compile_definitions(H_SHIFTER_JAPATECH PUBLIC
    USB_MANUFACTURER="Alex"
    USB_PRODUCT="Truck"
    USBD_MAX_POWER_MA=250
    ARDUINO_RASPBERRY_PI_PICO_2=1
  )

  pico_enable_stdio_usb(H_SHIFTER_JAPATECH 1)
  pico_add_extra_outputs(H_SHIFTER_JAPATECH)
else()
  message("Skipping TinyUSB device examples as TinyUSB is unavailable")
endif()
