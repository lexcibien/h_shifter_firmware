#pragma once

#include <Arduino.h>
#include <ShifterModel.h>

class Debug {
public:
  static void printRawInputs(const ShifterModel::InputState& inputs);
  static void printOutputState(const ShifterModel::ButtonState& buttonState);
};
