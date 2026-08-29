#pragma once

#include "hardware/adc.h"
#include <ShifterModel.h>
#include <waveshare_rp2040_zero.h>

class ShifterInput {
private:
  static constexpr uint8_t BITS_RES = 12;
  static constexpr uint16_t RANGE_RES = (1 << BITS_RES) - 1;
  static constexpr float REF_VOLTAGE = 3.3F;

  static constexpr uint16_t KNOB_RESISTOR = 20e3;
  static constexpr uint16_t UC_RESISTOR = 10e3;
  static constexpr float VOLT_DIVISOR_FACTOR = static_cast<float>(KNOB_RESISTOR) / (UC_RESISTOR + KNOB_RESISTOR);
  static constexpr float CONVERSION_FACTOR = REF_VOLTAGE / RANGE_RES;
  static constexpr auto TARGET_ANG_VALUE = static_cast<uint16_t>(RANGE_RES * VOLT_DIVISOR_FACTOR);

  // In the handle truck shifter, change the 1.07 MΩ original resistor for a 20 kΩ,
  // and at the uC, in the SW_KNOB_RANGE pin and the 3.3 V, use a 10 kΩ resistor.
  static bool detectHandleConnection() {
    adc_select_input(SW_KNOB_RANGE - 26);
    const uint16_t DETECT_VALUE = TARGET_ANG_VALUE + 700; // Needs to be less than 4095
    int adc = adc_read();
    return adc < DETECT_VALUE;
  }

  bool handleConnected = false;

public:
  struct AnalogState {
    int adc = 0;
    float voltage = 0.0F;
  };

  void begin();
  static ShifterInput::AnalogState readAnalogInput();
  [[nodiscard]] ShifterModel::InputState readInputs() const;
  [[nodiscard]] ShifterModel::ShifterConfig configuration() const;
};
