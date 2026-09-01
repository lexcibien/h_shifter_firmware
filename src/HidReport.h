#pragma once

#include "hid/Adafruit_USBD_HID.h"
#include <ShifterModel.h>

class HidReport {
private:
  static constexpr uint8_t HID_BUTTON_COUNT = ShifterModel::BUTTON_COUNT;
  struct __attribute__((packed)) ButtonsReport {
    std::array<uint8_t, 2> buttons;
  };

  Adafruit_USBD_HID usbHid;
  ButtonsReport buttonReport = {};
  static constexpr std::array<uint8_t, 29> hidReportDescriptor = {
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),     //
    HID_USAGE(HID_USAGE_DESKTOP_JOYSTICK),      //
    HID_COLLECTION(HID_COLLECTION_APPLICATION), //

    HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON), //
    HID_USAGE_MIN(1),                      //
    HID_USAGE_MAX(HID_BUTTON_COUNT),       //

    HID_LOGICAL_MIN(0), //
    HID_LOGICAL_MAX(1), //

    HID_REPORT_SIZE(1),                                //
    HID_REPORT_COUNT(HID_BUTTON_COUNT),                //
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), //

    // Padding: 4 bits
    HID_REPORT_SIZE(4),      //
    HID_REPORT_COUNT(1),     //
    HID_INPUT(HID_CONSTANT), //

    HID_COLLECTION_END //
  };

public:
  void begin();

  static bool mounted();
  bool ready();

  void send(const ShifterModel::ButtonState& buttonState);
};
