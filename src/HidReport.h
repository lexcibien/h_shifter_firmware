#pragma once

#include "tusb.h"
#include <ShifterModel.h>

class HidReport {
private:
  static constexpr uint8_t HID_BUTTON_COUNT = ShifterModel::BUTTON_COUNT;
  struct __attribute__((packed)) ButtonsReport {
    std::array<uint8_t, 2> buttons;
  };

  ButtonsReport buttonReport = {};
  static constexpr std::array<uint8_t, 30> hidReportDescriptor = {
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

public:
  void begin();

  static bool mounted();
  static bool ready();

  void send(const ShifterModel::ButtonState& buttonState);
};
