add_executable(H_SHIFTER_JAPATECH)

target_sources(H_SHIFTER_JAPATECH PRIVATE
  main.cpp

  app/ShifterApp.cpp

  config/DeviceConfig.cpp
  storage/ConfigStore.cpp

  shifter/ShifterInput.cpp
  shifter/ShifterLogic.cpp

  usb/HidReport.cpp
  usb/WebUsbProtocol.cpp
  usb/usb_descriptors.c

  led/StatusLed.cpp
  led/Ws2812Led.cpp

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
  PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR=0
  H_SHIFTER_WEBUSB_URL="${H_SHIFTER_WEBUSB_URL}"
)

target_link_libraries(H_SHIFTER_JAPATECH PRIVATE
  pico_usb_reset
  hardware_pio
  pico_stdlib
  tinyusb_device
  tinyusb_board
  hardware_adc
  hardware_flash
  hardware_watchdog
  pico_flash
)

# Reserve the final two flash sectors in the linker only. The SDK flash
# functions use the board's full PICO_FLASH_SIZE_BYTES to validate writes.
math(EXPR PICO_FLASH_SIZE_BYTES_STRING "${PICO_FLASH_SIZE_BYTES} - (8 * 1024)")
set(shifter_flash_region "${CMAKE_CURRENT_BINARY_DIR}/flash_layout")
configure_file(
  "${PICO_SDK_PATH}/src/rp2_common/pico_standard_link/pico_flash_region.template.ld"
  "${shifter_flash_region}/pico_flash_region.ld"
)
pico_add_linker_script_override_path(H_SHIFTER_JAPATECH "${shifter_flash_region}" FILES pico_flash_region.ld)
target_compile_definitions(H_SHIFTER_JAPATECH PRIVATE H_SHIFTER_CONFIG_FLASH_OFFSET=${PICO_FLASH_SIZE_BYTES_STRING})

pico_generate_pio_header(H_SHIFTER_JAPATECH
  ${CMAKE_CURRENT_LIST_DIR}/led/ws2812.pio
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
