#include <Arduino.h>
#include <ShifterInput.h>

void ShifterInput::begin() {
  pinMode(SW_FRONT, INPUT_PULLUP);
  pinMode(SW_LEFT, INPUT_PULLUP);
  pinMode(SW_RIGHT, INPUT_PULLUP);
  pinMode(SW_BACK, INPUT_PULLUP);
  pinMode(SW_REVERSE, INPUT_PULLUP);
  pinMode(SW_ENABLE_SEQUENTIAL, INPUT_PULLUP);

  analogReadResolution(ADC_RESOLUTION);

  delay(2000);
  handleConnected = detectHandleConnection();

  if (handleConnected) {
    configureHandle();
  }
}

bool ShifterInput::detectHandleConnection() {
  const uint16_t DETECT_VALUE = TARGET_ANG_VALUE + 700; // Needs to be less than 4095
  int adc = analogRead(SW_KNOB_RANGE);

  return adc < DETECT_VALUE;
}

void ShifterInput::configureHandle() {
  pinMode(SW_KNOB_RANGE, INPUT);
  pinMode(SW_KNOB_SPLIT, INPUT_PULLUP);
  pinMode(BTN_KNOB_ENGINE_BRAKE, INPUT_PULLUP);
}

void ShifterInput::checkSequential() {
  if (const bool isPressed = digitalRead(SW_ENABLE_SEQUENTIAL) == LOW; !isPressed) {
    sequentialPressActive = false;
    sequentialPressStartTime = 0;
    return;
  }

  if (!sequentialPressActive) {
    sequentialPressActive = true;
    sequentialPressStartTime = millis();
    return;
  }

  if ((millis() - sequentialPressStartTime) >= SEQUENTIAL_MODE_HOLD_TIME_MS) {
    sequentialEnabled = !sequentialEnabled;
    sequentialPressActive = false;
    sequentialPressStartTime = 0;
  }
}

ShifterInput::AnalogState ShifterInput::readAnalogInput() {
  ShifterInput::AnalogState input;

  input.adc = analogRead(SW_KNOB_RANGE);
  input.voltage = static_cast<float>(input.adc) * CONVERSION_FACTOR;

  return input;
}

ShifterModel::InputState ShifterInput::readInputs() {
  ShifterModel::InputState inputs;

  inputs.swFront = digitalRead(SW_FRONT) == LOW;
  inputs.swLeft = digitalRead(SW_LEFT) == LOW;
  inputs.swRight = digitalRead(SW_RIGHT) == LOW;
  inputs.swBack = digitalRead(SW_BACK) == LOW;
  inputs.swReverse = digitalRead(SW_REVERSE) == HIGH;

  checkSequential();

  if (millis() - lastScan >= 500) {
    handleConnected = detectHandleConnection();
    if (handleConnected) {
      configureHandle();
    }
    lastScan = millis();
  }

  if (handleConnected) {
    inputs.swRange = digitalRead(SW_KNOB_RANGE) == LOW;
    inputs.swSplit = digitalRead(SW_KNOB_SPLIT) == LOW;
    inputs.btnEngineBrake = digitalRead(BTN_KNOB_ENGINE_BRAKE) == LOW;
  } else {
    inputs.swRange = LOW;
    inputs.swSplit = LOW;
    inputs.btnEngineBrake = LOW;
  }

  return inputs;
}

ShifterModel::ShifterConfig ShifterInput::configuration() { return { .sequentialEnabled = sequentialEnabled, .handleConnected = handleConnected }; }
