#include <Debug.h>
#include <HidReport.h>
#include <ShifterInput.h>
#include <ShifterLogic.h>
#include <ShifterModel.h>
#include <cstdio>
#include <pico/stdlib.h>
#include <tusb.h>

int main() {
  HidReport hidReport;
  ShifterInput shifterInput;
  ShifterLogic shifterLogic;
  ShifterModel::ButtonState previousButtonState = {};
  ShifterModel::ShifterConfig shifterConfig;

  stdio_init_all();
  HidReport::begin();
  shifterInput.begin();
  shifterConfig = shifterInput.configuration();

  if (!shifterConfig.handleConnected) {
    printf("INFO: Truck shifter handle not connected\n");
  } else {
    printf("OK: Handle detected");
  }

  printf(ShifterModel::ENABLE_REVERSE ? "OK: Rear gear is enabled" : "INFO: Rear gear is disabled");
  printf(shifterConfig.sequentialEnabled ? "OK: The current gear output is sequential" : "INFO: The current gear output is H-Shifter");

  while (true) {
#ifdef TINYUSB_NEED_POLLING_TASK
    tud_task();
#endif

    if (!HidReport::mounted()) {
      sleep_ms(10);
      continue;
    }

    ShifterModel::InputState inputs = shifterInput.readInputs();

#ifdef DEBUG
#ifdef DEBUG_ANALOG
  ShifterInput::AnalogState inputAnalog = ShifterInput::readAnalogInput(); //* The button will not work digitally
  Debug::printRawAnalog(inputAnalog);
#endif
  Debug::printRawInputs(inputs);
#endif

    ShifterModel::ButtonState buttonState = shifterLogic.resolveButtonState(inputs, shifterConfig);

    if (!HidReport::ready()) {
      sleep_ms(10);
      continue;
    }

    if (buttonState != previousButtonState) {
      hidReport.send(buttonState);
#ifdef DEBUG
      Debug::printOutputState(buttonState);
#endif
      previousButtonState = buttonState;
    }

    sleep_ms(20);
  }
}
