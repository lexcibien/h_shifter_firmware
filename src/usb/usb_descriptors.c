#include "config/ShifterProject.h"
#include <bsp/board_api.h>

#include <pico/usb_reset_interface.h>
#include <pico/usb_reset_tusb.h>

#define USB_VID 0x2E8A
#define USB_PID 0x0003
#define USB_BCD 0x0210

#define VENDOR_REQUEST_WEBUSB    0x21
#define VENDOR_REQUEST_MICROSOFT 0x22
#define WEBUSB_URL_INDEX         1

enum { STRID_LANGID, STRID_MANUFACTURER, STRID_PRODUCT, STRID_SERIAL, STRID_WEBUSB, STRID_LOG, STRID_CONFIG };

#if CFG_TUD_CDC
#define DEVICE_CLASS    TUSB_CLASS_MISC
#define DEVICE_SUBCLASS MISC_SUBCLASS_COMMON
#define DEVICE_PROTOCOL MISC_PROTOCOL_IAD
#else
#define DEVICE_CLASS    TUSB_CLASS_UNSPECIFIED
#define DEVICE_SUBCLASS 0
#define DEVICE_PROTOCOL 0
#endif

// NOLINTNEXTLINE(misc-use-internal-linkage)
tusb_desc_device_t const desc_device = {
  .bLength = sizeof(tusb_desc_device_t),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = USB_BCD,
  .bDeviceClass = DEVICE_CLASS,
  .bDeviceSubClass = DEVICE_SUBCLASS,
  .bDeviceProtocol = DEVICE_PROTOCOL,
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor = USB_VID,
  .idProduct = USB_PID,
  .bcdDevice = 0x0200,
  .iManufacturer = 0x01,
  .iProduct = 0x02,
  .iSerialNumber = 0x03,
  .bNumConfigurations = 0x01,
};

uint8_t const* tud_descriptor_device_cb(void) { return (uint8_t const*)&desc_device; }

// NOLINTBEGIN
uint8_t const desc_hid_report[] = {
  HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),     //
  HID_USAGE(HID_USAGE_DESKTOP_JOYSTICK),      //
  HID_COLLECTION(HID_COLLECTION_APPLICATION), //

  /* Buttons */
  HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON), //
  HID_USAGE_MIN(1),                      //
  HID_USAGE_MAX(SHIFTER_BUTTON_COUNT),   //

  HID_LOGICAL_MIN(0), //
  HID_LOGICAL_MAX(1), //

  HID_REPORT_SIZE(1),                                //
  HID_REPORT_COUNT(SHIFTER_BUTTON_COUNT),            //
  HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), //

  /* Padding */
  HID_REPORT_SIZE(SHIFTER_HID_PADDING_COUNT), //
  HID_REPORT_COUNT(1),                        //
  HID_INPUT(HID_CONSTANT),                    //

  HID_COLLECTION_END //
};

uint16_t const desc_hid_report_size = sizeof(desc_hid_report);

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return desc_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}

enum {
#if H_SHIFTER_LOG
  ITF_NUM_LOG,
  ITF_NUM_LOG_DATA,
#endif
  ITF_NUM_HID,
  ITF_NUM_WEBUSB,
  ITF_NUM_RESET,
#if H_SHIFTER_CONFIG_CDC
  ITF_NUM_CONFIG,
  ITF_NUM_CONFIG_DATA,
#endif
  ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN + TUD_VENDOR_DESC_LEN + TUD_RPI_RESET_DESC_LEN)

#define EPNUM_HID          0x81
#define EPNUM_LOG_NOTIF    0x82
#define EPNUM_LOG_OUT      0x03
#define EPNUM_LOG_IN       0x83
#define EPNUM_WEBUSB_OUT   0x04
#define EPNUM_WEBUSB_IN    0x84
#define EPNUM_CONFIG_NOTIF 0x85
#define EPNUM_CONFIG_OUT   0x06
#define EPNUM_CONFIG_IN    0x86

uint8_t const desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
#if H_SHIFTER_LOG
  TUD_CDC_DESCRIPTOR(ITF_NUM_LOG, STRID_LOG, EPNUM_LOG_NOTIF, 8, EPNUM_LOG_OUT, EPNUM_LOG_IN, 64),
#endif
  TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, 2, 5),
  TUD_VENDOR_DESCRIPTOR(ITF_NUM_WEBUSB, STRID_WEBUSB, EPNUM_WEBUSB_OUT, EPNUM_WEBUSB_IN, 64),
  TUD_RPI_RESET_DESCRIPTOR(ITF_NUM_RESET, 0),
#if H_SHIFTER_CONFIG_CDC
  TUD_CDC_DESCRIPTOR(ITF_NUM_CONFIG, STRID_CONFIG, EPNUM_CONFIG_NOTIF, 8, EPNUM_CONFIG_OUT, EPNUM_CONFIG_IN, 64),
#endif
};
TU_VERIFY_STATIC(sizeof(desc_configuration) == CONFIG_TOTAL_LEN, "Incorrect USB configuration descriptor size");

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

// WebUSB and Microsoft OS 2.0 descriptors. The latter binds only the two
// vendor-specific functions to WinUSB while the HID interface remains owned
// by the operating system's joystick driver.
#define BOS_TOTAL_LEN        (TUD_BOS_DESC_LEN + TUD_BOS_WEBUSB_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)
#define WEBUSB_PROPERTY_NAME u"DeviceInterfaceGUIDs"
// REG_MULTI_SZ needs an extra NUL after the GUID string's terminator.
#define WEBUSB_PROPERTY_DATA u"{1C3DA97A-88B7-4E5F-9AE0-2A57510D6430}\0"
#define WEBUSB_PROPERTY_LEN  (0x0A + sizeof(WEBUSB_PROPERTY_NAME) + sizeof(WEBUSB_PROPERTY_DATA))
#define WEBUSB_FUNCTION_LEN  (0x08 + 0x14 + WEBUSB_PROPERTY_LEN)
#define MS_OS_20_DESC_LEN    (0x0A + 0x08 + WEBUSB_FUNCTION_LEN + RPI_RESET_MS_OS_20_DESC_LEN)

uint8_t const desc_bos[] = {
  TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 2),
  TUD_BOS_WEBUSB_DESCRIPTOR(VENDOR_REQUEST_WEBUSB, WEBUSB_URL_INDEX),
  TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, VENDOR_REQUEST_MICROSOFT),
};
TU_VERIFY_STATIC(sizeof(desc_bos) == BOS_TOTAL_LEN, "Incorrect BOS descriptor size");

uint8_t const* tud_descriptor_bos_cb(void) { return desc_bos; }

// The RP2040 stores the UTF-16 code units in little-endian order.
struct TU_ATTR_PACKED {
  uint8_t set_header[0x0A];
  uint8_t configuration_header[0x08];
  uint8_t function_header[0x08];
  uint8_t compatible_id_header[0x04];
  char compatible_id[8];
  char sub_compatible_id[8];
  uint8_t property_header[0x08];
  uint16_t property_name[sizeof(WEBUSB_PROPERTY_NAME) / sizeof(uint16_t)];
  uint8_t property_data_length[2];
  uint16_t property_data[sizeof(WEBUSB_PROPERTY_DATA) / sizeof(uint16_t)];
  uint8_t reset_function[RPI_RESET_MS_OS_20_DESC_LEN];
} const desc_ms_os_20 = {
  .set_header = {
    U16_TO_U8S_LE(0x000A),
    U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
    U32_TO_U8S_LE(0x06030000),
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN),
  },
  .configuration_header = {
    U16_TO_U8S_LE(0x0008),
    U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION),
    0,
    0,
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),
  },
  .function_header = {
    U16_TO_U8S_LE(0x0008),
    U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION),
    ITF_NUM_WEBUSB,
    0,
    U16_TO_U8S_LE(WEBUSB_FUNCTION_LEN),
  },
  .compatible_id_header = {
    U16_TO_U8S_LE(0x0014),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
  },
  .compatible_id = "WINUSB",
  .sub_compatible_id = { 0 },
  .property_header = {
    U16_TO_U8S_LE(WEBUSB_PROPERTY_LEN),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    U16_TO_U8S_LE(0x0007), // REG_MULTI_SZ
    U16_TO_U8S_LE(sizeof(WEBUSB_PROPERTY_NAME)),
  },
  .property_name = WEBUSB_PROPERTY_NAME,
  .property_data_length = { U16_TO_U8S_LE(sizeof(WEBUSB_PROPERTY_DATA)) },
  .property_data = WEBUSB_PROPERTY_DATA,
  .reset_function = { RPI_RESET_MS_OS_20_DESCRIPTOR(ITF_NUM_RESET) },
};

TU_VERIFY_STATIC(sizeof(desc_ms_os_20) == MS_OS_20_DESC_LEN, "Incorrect Microsoft OS 2.0 descriptor size");

#ifndef H_SHIFTER_WEBUSB_URL
#define H_SHIFTER_WEBUSB_URL "lexcibien.github.io/h_shifter_firmware/"
#endif

#define WEBUSB_URL H_SHIFTER_WEBUSB_URL
const tusb_desc_webusb_url_t desc_url = {
  .bLength = 3 + sizeof(WEBUSB_URL) - 1,
  .bDescriptorType = 3,
  .bScheme = 1,
  .url = WEBUSB_URL,
};

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {
  if (stage != CONTROL_STAGE_SETUP) {
    return true;
  }
  if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR || request->bmRequestType_bit.recipient != TUSB_REQ_RCPT_DEVICE) {
    return false;
  }
  if (request->bRequest == VENDOR_REQUEST_WEBUSB && request->wIndex == 2 && request->wValue == WEBUSB_URL_INDEX) {
    return tud_control_xfer(rhport, request, (void*)(uintptr_t)&desc_url, desc_url.bLength);
  }
  if (request->bRequest == VENDOR_REQUEST_MICROSOFT && request->wIndex == 7) {
    return tud_control_xfer(rhport, request, (void*)(uintptr_t)&desc_ms_os_20, sizeof(desc_ms_os_20));
  }
  return false;
}

char const* string_desc_arr[] = {
  (const char[]){ 0x09, 0x04 }, USB_MANUFACTURER, USB_PRODUCT, NULL, "DAZ Shifter WebUSB", "DAZ Shifter Logs", "DAZ Shifter Config",
};

static uint16_t desc_string[32 + 1];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  size_t char_count = 0;

  if (index == STRID_LANGID) {
    memcpy(&desc_string[1], string_desc_arr[0], 2);
    char_count = 1;
  } else if (index == STRID_SERIAL) {
    char_count = board_usb_get_serial(desc_string + 1, 32);
  } else {
    if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
      return NULL;
    }
    char const* string = string_desc_arr[index];
    char_count = strnlen(string, 32);

    for (size_t i = 0; i < char_count; ++i) {
      desc_string[1 + i] = string[i];
    }
  }

  desc_string[0] = (uint16_t)((TUSB_DESC_STRING << 8) | ((2 * char_count) + 2));
  return desc_string;
}
// NOLINTEND
