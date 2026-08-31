#include <ShifterInput.h>

void ShifterInput::begin() {
  pinMode(SW_FRONT, INPUT_PULLUP);
  pinMode(SW_LEFT, INPUT_PULLUP);
  pinMode(SW_RIGHT, INPUT_PULLUP);
  pinMode(SW_BACK, INPUT_PULLUP);
  pinMode(SW_REVERSE, INPUT_PULLUP);
  pinMode(SW_ENABLE_SEQUENTIAL, INPUT_PULLUP);

  analogReadResolution(BITS_RES);

  delay(2000);
  handleConnected = detectHandleConnection();

  if (handleConnected) {
    configureHandle();
  }
}

void ShifterInput::configureHandle() {
  pinMode(SW_KNOB_RANGE, INPUT);
  pinMode(SW_KNOB_SPLIT, INPUT_PULLUP);
  pinMode(BTN_KNOB_ENGINE_BRAKE, INPUT_PULLUP);
}

void ShifterInput::checkSequential() {
  bool reading = digitalRead(SW_ENABLE_SEQUENTIAL);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        sequentialEnabled = !sequentialEnabled;
      }
    }
  }

  lastButtonState = reading;
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

  if (millis() - lastScan >= 2000) {
    handleConnected = detectHandleConnection();
    checkSequential();
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

ShifterModel::ShifterConfig ShifterInput::configuration() {
  return { .sequentialEnabled = sequentialEnabled, .handleConnected = handleConnected };
}
