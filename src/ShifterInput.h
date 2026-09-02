#pragma once

#include <Arduino.h>
#include <ShifterModel.h>
#include <board.h>

class ShifterInput {
private:
  static constexpr uint16_t RANGE_RES = (1 << ADC_RESOLUTION) - 1;
  static constexpr float REF_VOLTAGE = 3.3F;

  static constexpr uint16_t KNOB_RESISTOR = 20e3;
  static constexpr uint16_t UC_RESISTOR = 10e3;
  static constexpr float VOLT_DIVISOR_FACTOR = static_cast<float>(KNOB_RESISTOR) / (UC_RESISTOR + KNOB_RESISTOR);
  static constexpr float CONVERSION_FACTOR = REF_VOLTAGE / RANGE_RES;
  static constexpr auto TARGET_ANG_VALUE = static_cast<uint16_t>(RANGE_RES * VOLT_DIVISOR_FACTOR);

  // In the handle truck shifter, change the 1.07 MΩ original resistor for a 20 kΩ,
  // and at the uC, in the SW_KNOB_RANGE pin and the 3.3 V, use a 10 kΩ resistor.
  static bool detectHandleConnection() {
    const uint16_t DETECT_VALUE = TARGET_ANG_VALUE + 700; // Needs to be less than 4095
    int adc = analogRead(SW_KNOB_RANGE);

    return adc < DETECT_VALUE;
  }

  inline static bool handleConnected = false;
  inline static bool sequentialEnabled = false;
  inline static uint32_t lastScan;

  inline static bool sequentialPressActive = false;
  inline static uint32_t sequentialPressStartTime = 0;
  static constexpr uint32_t SEQUENTIAL_MODE_HOLD_TIME_MS = 1000;

public:
  struct AnalogState {
    int adc = 0;
    float voltage = 0.0F;
  };

  static void begin();
  static void configureHandle();
  static void checkSequential();
  static ShifterInput::AnalogState readAnalogInput();
  static ShifterModel::InputState readInputs();
  static ShifterModel::ShifterConfig configuration();
};
