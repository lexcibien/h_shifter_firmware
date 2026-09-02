#include <Arduino.h>
#include <Debug.h>
#include <HidReport.h>
#include <LedMulti.h>
#include <ShifterInput.h>
#include <ShifterLogic.h>
#include <ShifterModel.h>

// NOLINTBEGIN (cppcoreguidelines-avoid-non-const-global-variables)
HidReport hidReport;
ShifterInput shifterInput;
Debug debug;
ShifterModel::ButtonState previousButtonState = {};
ShifterModel::ShifterConfig shifterConfig;
LedMulti ledMulti;
// NOLINTEND

void setup() {
  Serial.begin(115200);
  hidReport.begin();
  ledMulti.begin();
  ShifterInput::begin();

  shifterConfig = ShifterInput::configuration();

  if (!shifterConfig.handleConnected) {
    Serial.println("INFO: Truck shifter handle not connected");
  } else {
    Serial.println("OK: Handle detected");
  }

  const char* gearType = "";
  if (shifterConfig.sequentialEnabled) {
    ledMulti.showLed(Color::SEQUENTIAL);
    gearType = "Sequential";
  } else {
    ledMulti.showLed(Color::H_SHIFTER);
    gearType = "H Shifter";
  }

  Serial.println(ShifterModel::ENABLE_REVERSE ? "OK: Rear gear is enabled" : "INFO: Rear gear is disabled");
  Serial.printf("OK: The current gear output is %s", gearType);
}

void loop() {
  static bool executeOnce = true;
#ifdef TINYUSB_NEED_POLLING_TASK
  TinyUSBDevice.task();
#endif

  if (!HidReport::mounted()) {
    return;
  }

  ShifterModel::InputState inputs = ShifterInput::readInputs();

  if (!LedMulti::blockSignal) {
    if (shifterConfig.sequentialEnabled) {
      ledMulti.showLed(Color::SEQUENTIAL);
    } else {
      ledMulti.showLed(Color::H_SHIFTER);
    }
  }

  if (shifterConfig.handleConnected) {
    if (executeOnce) {
      ledMulti.connectHandle();
      executeOnce = false;
    }
  } else {
    executeOnce = true;
  }
  shifterConfig = ShifterInput::configuration();

#ifdef DEBUG
#ifdef DEBUG_ANALOG
  ShifterInput::AnalogState inputAnalog = ShifterInput::readAnalogInput(); //* The button will not work digitally
  Debug::printRawAnalog(inputAnalog);
#endif
  Debug::printRawInputs(inputs);
#endif

  ShifterModel::ButtonState buttonState = ShifterLogic::resolveButtonState(inputs, shifterConfig);

  if (!hidReport.ready()) {
    return;
  }

  if (buttonState != previousButtonState) {
    hidReport.send(buttonState);
#ifdef DEBUG
    Debug::printOutputState(buttonState);
#endif
    previousButtonState = buttonState;
  }

  ledMulti.update();
  delay(20);
}
