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
  pico_stdlib
  hardware_pio
)

set_source_files_properties(Adafruit_NeoPixel.cpp PROPERTIES
  COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic"
)

