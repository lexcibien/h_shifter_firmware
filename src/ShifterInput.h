#pragma once

#include <Arduino.h>
#include <ShifterModel.h>

class ShifterInput {
private:
  static bool detectHandleConnection() {
    int adc = analogRead(SW_KNOB_RANGE);
    return adc > 300 && adc < 600;
  }

  bool handleConnected = false;

public:
  void begin();

  [[nodiscard]] ShifterModel::InputState readInputs() const;

  [[nodiscard]] ShifterModel::ShifterConfig configuration() const;
};
