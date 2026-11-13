add_executable(H_SHIFTER_JAPATECH)

target_sources(H_SHIFTER_JAPATECH PRIVATE
  main.cpp

  app/ShifterApp.cpp

  shifter/ShifterInput.cpp
  shifter/ShifterLogic.cpp

  usb/HidReport.cpp
  usb/usb_descriptors.c

  led/StatusLed.cpp

  diagnostics/Log.cpp
  diagnostics/Debug.cpp
)

target_include_directories(H_SHIFTER_JAPATECH PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/usb
)

target_compile_options(H_SHIFTER_JAPATECH PRIVATE
  -Wall
  -Wno-unused-function # we have some for the docs that aren't called
  -fstack-usage
)
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
  PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE=1
  PICO_USB_RESET_INCLUDE_DEFAULT_APP_DRIVER_CB=1
)

target_link_libraries(H_SHIFTER_JAPATECH PRIVATE
  Adafruit::NeoPixel
  pico_usb_reset
  hardware_pio
  pico_stdlib
  tinyusb_device
  tinyusb_board
  hardware_adc
)

pico_enable_stdio_usb(H_SHIFTER_JAPATECH ${H_SHIFTER_LOG})
pico_enable_stdio_uart(H_SHIFTER_JAPATECH 0)

set_target_properties(H_SHIFTER_JAPATECH PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

pico_add_extra_outputs(H_SHIFTER_JAPATECH)

h_shifter_add_post_build_steps(H_SHIFTER_JAPATECH)
h_shifter_add_upload_target(H_SHIFTER_JAPATECH)
