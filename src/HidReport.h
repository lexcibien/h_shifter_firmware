#pragma once

#include "tusb.h"
#include <usb_descriptors.h>
#include <ShifterModel.h>

class HidReport {
private:
  // static constexpr uint8_t HID_BUTTON_COUNT = ShifterModel::BUTTON_COUNT;
  struct __attribute__((packed)) ButtonsReport {
    std::array<uint8_t, 2> buttons;
  };

  ButtonsReport buttonReport = {};

public:
  void begin();

  static bool mounted();
  static bool ready();

  void send(const ShifterModel::ButtonState& buttonState);
};
