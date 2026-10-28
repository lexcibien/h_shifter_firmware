#pragma once

#include <ShifterModel.h>
#include <board.h>
#include <pins_arduino.h>

class ShifterInput {
private:
  struct Button {
    uint8_t pin;
    bool activeLow;
    bool lastRaw;
    bool stableState;
    uint32_t lastChange;
  };

  static constexpr uint32_t DEBOUNCE_MS = 15;

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  inline static Button frontButton = { .pin = SW_FRONT, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0 };
  inline static Button leftButton = { .pin = SW_LEFT, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0 };
  inline static Button rightButton = { .pin = SW_RIGHT, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0 };
  inline static Button backButton = { .pin = SW_BACK, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0 };
  inline static Button reverseButton = { .pin = SW_REVERSE, .activeLow = false, .lastRaw = false, .stableState = false, .lastChange = 0 };
  inline static Button sequentialModeButton = {
    .pin = SW_ENABLE_SEQUENTIAL, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0,
  };
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  inline static Button modeButton = { .pin = PIN_MODE, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0 };
  inline static Button button3 = { .pin = PIN_3, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0 };
  inline static Button button4 = { .pin = PIN_4, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0 };
  inline static Button button5 = { .pin = PIN_5, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0 };
  inline static Button button6 = { .pin = PIN_6, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0 };
#endif

  inline static Button rangeButton = { .pin = SW_KNOB_RANGE, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0 };
  inline static Button splitButton = { .pin = SW_KNOB_SPLIT, .activeLow = true, .lastRaw = false, .stableState = false, .lastChange = 0 };
  inline static Button engineBrakeButton = {
    .pin = BTN_KNOB_ENGINE_BRAKE,
    .activeLow = true,
    .lastRaw = false,
    .stableState = false,
    .lastChange = 0,
  };

  static constexpr uint16_t RANGE_RES = (1 << ADC_RESOLUTION) - 1;
  static constexpr float REF_VOLTAGE = 3.3F;

  static constexpr uint16_t KNOB_RESISTOR = 20e3;
  static constexpr uint16_t UC_RESISTOR = 10e3;
  static constexpr float VOLT_DIVISOR_FACTOR = static_cast<float>(KNOB_RESISTOR) / (UC_RESISTOR + KNOB_RESISTOR);
  static constexpr float CONVERSION_FACTOR = REF_VOLTAGE / RANGE_RES;
  static constexpr auto TARGET_ANG_VALUE = static_cast<uint16_t>(RANGE_RES * VOLT_DIVISOR_FACTOR);

  // In the handle truck shifter, change the 1.07 MΩ original resistor for a 20 kΩ,
  // and at the uC, in the SW_KNOB_RANGE pin and the 3.3 V, use a 10 kΩ resistor.
  static bool detectHandleConnection();
  static bool readDebounced(Button& button);

  inline static bool handleConnected = false;
  inline static bool sequentialEnabled = false;
  inline static uint32_t lastScan;

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  inline static bool sequentialPressActive = false;
  inline static uint32_t sequentialPressStartTime = 0;
  static constexpr uint32_t SEQUENTIAL_MODE_HOLD_TIME_MS = 1000;
#endif

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
