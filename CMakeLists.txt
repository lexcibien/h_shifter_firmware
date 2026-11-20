# Firmware para H-shifter (RP2040 Zero)

cmake_minimum_required(VERSION 3.35)

include(cmake/ShifterOptions.cmake)

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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(PICO_SDK_VERSION_STRING VERSION_LESS "2.3.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

include(cmake/FirmwareUtilities.cmake)

add_subdirectory("lib/Adafruit NeoPixel")
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
