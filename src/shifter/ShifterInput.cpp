#include "ShifterInput.h"
#include <Arduino.h>

bool ShifterInput::readDebounced(Button& button) {
  const bool raw = button.activeLow ? !digitalRead(button.pin) : digitalRead(button.pin);

  if (raw != button.lastRaw) {
    button.lastChange = nowMs;
    button.lastRaw = raw;
  }
  if ((nowMs - button.lastChange) > DEBOUNCE_MS) {
    button.stableState = raw;
  }

  return button.stableState;
}

void ShifterInput::begin() {
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  pinMode(SW_FRONT, INPUT_PULLUP);
  pinMode(SW_LEFT, INPUT_PULLUP);
  pinMode(SW_RIGHT, INPUT_PULLUP);
  pinMode(SW_BACK, INPUT_PULLUP);
  pinMode(SW_REVERSE, INPUT_PULLUP);
  pinMode(SW_ENABLE_SEQUENTIAL, INPUT_PULLUP);
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  pinMode(PIN_MODE, INPUT_PULLUP);
  pinMode(PIN_3, INPUT_PULLUP);
  pinMode(PIN_4, INPUT_PULLUP);
  pinMode(PIN_5, INPUT_PULLUP);
  pinMode(PIN_6, INPUT_PULLUP);
#endif
  analogReadResolution(ADC_RES);
  delay(2000);

  handleConnected = detectHandleConnection();

  if (handleConnected) {
    configureHandle();
  }
}

bool ShifterInput::detectHandleConnection() {
  const int DETECT_VALUE = TARGET_ANG_VALUE + 700; // Needs to be less than 4095
  const int adc = analogRead(SW_KNOB_RANGE);

  return adc < DETECT_VALUE;
}

void ShifterInput::configureHandle() {
  pinMode(SW_KNOB_RANGE, INPUT);
  pinMode(SW_KNOB_SPLIT, INPUT_PULLUP);
  pinMode(BTN_KNOB_ENGINE_BRAKE, INPUT_PULLUP);
}

void ShifterInput::checkSequential() {
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  if (const bool isPressed = readDebounced(sequentialModeButton); !isPressed) {
    sequentialPressActive = false;
    sequentialPressHandled = false;
    sequentialPressStartTime = 0;
    return;
  }

  if (!sequentialPressActive) {
    sequentialPressActive = true;
    sequentialPressStartTime = nowMs;
    return;
  }

  if (!sequentialPressHandled && (nowMs - sequentialPressStartTime) >= SEQUENTIAL_MODE_HOLD_TIME_MS) {
    sequentialEnabled = !sequentialEnabled;
    sequentialPressHandled = true;
  }
#endif
}

ShifterInput::AnalogState ShifterInput::readAnalogInput() {
  ShifterInput::AnalogState input;

  input.adc = analogRead(SW_KNOB_RANGE);
  input.voltage = static_cast<float>(input.adc) * CONVERSION_FACTOR;

  return input;
}

ShifterModel::InputState ShifterInput::readInputs() {
  ShifterModel::InputState inputs;
  nowMs = millis();

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  inputs.swFront = readDebounced(frontButton);
  inputs.swLeft = readDebounced(leftButton);
  inputs.swRight = readDebounced(rightButton);
  inputs.swBack = readDebounced(backButton);
  inputs.swReverse = readDebounced(reverseButton);

  checkSequential();
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  inputs.hPatternMode = readDebounced(modeButton);
  inputs.sw3 = readDebounced(button3);
  inputs.sw4 = readDebounced(button4);
  inputs.sw5 = readDebounced(button5);
  inputs.sw6 = readDebounced(button6);
  sequentialEnabled = !inputs.hPatternMode;
#endif

  if (handleConnected) {
    inputs.swRange = readDebounced(rangeButton);
    inputs.swSplit = readDebounced(splitButton);
    inputs.btnEngineBrake = readDebounced(engineBrakeButton);
  }

  return inputs;
}

ShifterModel::ShifterConfig ShifterInput::configuration() { return { .sequentialEnabled = sequentialEnabled, .handleConnected = handleConnected }; }
