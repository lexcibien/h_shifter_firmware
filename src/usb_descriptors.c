#include <string.h>

#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include <pico/stdlib.h>

#define USB_VID 0x2E8A
#define USB_PID 0x0003
#define USB_BCD 0x0200

tusb_desc_device_t const desc_device = {
  .bLength = sizeof(tusb_desc_device_t),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = USB_BCD,
  .bDeviceClass = 0,
  .bDeviceSubClass = 0,
  .bDeviceProtocol = 0,
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor = USB_VID,
  .idProduct = USB_PID,
  .bcdDevice = 0x0100,
  .iManufacturer = 0x01,
  .iProduct = 0x02,
  .iSerialNumber = 0x03,
  .bNumConfigurations = 0x01,
};

uint8_t const* tud_descriptor_device_cb(void) { return (uint8_t const*)&desc_device; }

uint8_t const desc_hid_report[] = {
  0x05, 0x01, // Usage Page (Generic Desktop)
  0x09, 0x04, // Usage (Joystick)
  0xA1, 0x01, // Collection (Application)

  0x05, 0x09,             // Usage Page (Button)
  0x19, 0x01,             // Usage Minimum (Button 1)
  0x29, HID_BUTTON_COUNT, // Usage Maximum (Button HID_BUTTON_COUNT)

  0x15, 0x00, // Logical Minimum (0)
  0x25, 0x01, // Logical Maximum (1)

  0x75, 0x01,             // Report Size (1)
  0x95, HID_BUTTON_COUNT, // Report Count (HID_BUTTON_COUNT)
  0x81, 0x02,             // Input (Data, Variable, Absolute)

  // Padding: 4 bits
  0x75, 0x04, // Report Size (4)
  0x95, 0x01, // Report Count (1)
  0x81, 0x01, // Input (Constant)

  0xC0 // End Collection
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

enum { ITF_NUM_HID, ITF_NUM_TOTAL };

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_HID        0x81

uint8_t const desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
  TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5),
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

enum { STRID_LANGID, STRID_MANUFACTURER, STRID_PRODUCT, STRID_SERIAL };

char const* string_desc_arr[] = {
  (const char[]){ 0x09, 0x04 },
  USB_MANUFACTURER,
  USB_PRODUCT,
  NULL,
};

static uint16_t desc_string[32 + 1];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  size_t char_count;

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
    char_count = strlen(string);
    if (char_count > 32) char_count = 32;
    for (size_t i = 0; i < char_count; ++i) {
      desc_string[1 + i] = string[i];
    }
  }

  desc_string[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * char_count + 2));
  return desc_string;
}
