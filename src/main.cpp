#include <Debug.h>
#include <HidReport.h>
#include <ShifterInput.h>
#include <ShifterLogic.h>
#include <ShifterModel.h>

// NOLINTBEGIN (cppcoreguidelines-avoid-non-const-global-variables)
HidReport hidReport;
ShifterInput shifterInput;
ShifterLogic shifterLogic;
Debug debug;
ShifterModel::ButtonState previousButtonState = {};
ShifterModel::ShifterConfig shifterConfig;
// NOLINTEND

void setup() {
  Serial.begin(115200);
  hidReport.begin();
  shifterInput.begin();
  shifterConfig = shifterInput.configuration();

  if (!shifterConfig.handleConnected) {
    Serial.println("INFO: Truck shifter handle not connected");
  } else {
    Serial.println("OK: Handle detected");
  }

  Serial.println(ShifterModel::ENABLE_REVERSE ? "OK: Rear gear is enabled" : "INFO: Rear gear is disabled");
  Serial.println(shifterConfig.sequentialEnabled ? "OK: The current gear output is sequential" : "INFO: The current gear output is H-Shifter");
}

void loop() {
#ifdef TINYUSB_NEED_POLLING_TASK
  TinyUSBDevice.task();
#endif

  if (!HidReport::mounted()) {
    return;
  }

  ShifterModel::InputState inputs = shifterInput.readInputs();

#ifdef DEBUG
  Debug::printRawInputs(inputs);
#endif

  ShifterModel::ButtonState buttonState = shifterLogic.resolveButtonState(inputs, shifterConfig);

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

  delay(20);
}
