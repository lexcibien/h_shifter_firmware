#pragma once

#include <ShifterModel.h>

class HidReport {
private:
  struct __attribute__((packed)) ButtonsReport {
    std::array<uint8_t, 2> buttons;
  };

  ButtonsReport buttonReport = {};

public:
  static void begin();

  static bool mounted();
  static bool ready();

  void send(const ShifterModel::ButtonState& buttonState);
};
