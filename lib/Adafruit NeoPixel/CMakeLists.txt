if(NOT PICO_PLATFORM STREQUAL "rp2040")
  message(FATAL_ERROR "Adafruit_NeoPixel supports only the RP2040 platform")
endif()

add_library(Adafruit_NeoPixel STATIC
  Adafruit_NeoPixel.cpp
)

add_library(Adafruit::NeoPixel ALIAS Adafruit_NeoPixel)

target_include_directories(Adafruit_NeoPixel
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Adafruit_NeoPixel
  PUBLIC
  hardware_pio_headers
  pico_time_headers
  PRIVATE
  hardware_clocks_headers
)

target_compile_options(Adafruit_NeoPixel PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -ffunction-sections
  -fdata-sections
  -fno-exceptions
  -fno-unwind-tables
  -fno-rtti
  -fno-use-cxa-atexit
)

