#pragma once

#include "hardware/adc.h"
#include <ShifterModel.h>
#include <waveshare_rp2040_zero.h>

class ShifterInput {
private:
  static bool detectHandleConnection() {
    adc_select_input(SW_KNOB_RANGE - 26);
    int adc = adc_read();
    return adc > 300 && adc < 600;
  }

  bool handleConnected = false;

public:
  void begin();

  [[nodiscard]] ShifterModel::InputState readInputs() const;

  [[nodiscard]] ShifterModel::ShifterConfig configuration() const;
};
