#pragma once

#include <Arduino.h>
#include <ShifterInput.h>
#include <ShifterModel.h>

class Debug {
public:
  static void printRawAnalog(const ShifterInput::AnalogState& input);
  static void printRawInputs(const ShifterModel::InputState& inputs);
  static void printOutputState(const ShifterModel::ButtonState& buttonState);
};
