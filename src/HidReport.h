#pragma once

#include <ShifterModel.h>

class HidReport {
private:
  struct __attribute__((packed)) ButtonsReport {
    std::array<uint8_t, SHIFTER_HID_REPORT_SIZE_BYTES> buttons;
  };

  ButtonsReport buttonReport = {};

public:
  static void begin();

  static bool mounted();
  static bool ready();

  void send(const ShifterModel::ButtonState& buttonState);
};
