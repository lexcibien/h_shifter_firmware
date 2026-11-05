#include <HidReport.h>
#include <ShifterInput.h>
#include <ShifterLogic.h>
#include <ShifterModel.h>
#include <cstdio>
#include <pico/stdio.h>
#include <pico/time.h>
#ifdef DEBUG
#include <Debug.h>
#endif
#ifdef TINYUSB_NEED_POLLING_TASK
#include <device/usbd.h>
#endif

int main() {
  HidReport hidReport;
  ShifterModel::ButtonState previousButtonState = {};
  ShifterModel::ShifterConfig shifterConfig;

  stdio_init_all();
  HidReport::begin();
  ShifterInput::begin();

  shifterConfig = ShifterInput::configuration();

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  puts("OK: Project version V1");
#else
  puts("OK: Project version V2");
#endif

  puts(shifterConfig.handleConnected ? "OK: Handle detected" : "INFO: Truck shifter handle not connected");
  puts(ShifterModel::ENABLE_REVERSE ? "OK: Rear gear is enabled" : "INFO: Rear gear is disabled");
  puts(shifterConfig.sequentialEnabled ? "OK: The current gear output is Sequential" : "OK: The current gear output is H Shifter");

  while (true) {
#ifdef TINYUSB_NEED_POLLING_TASK
    tud_task();
#endif

    if (!HidReport::mounted()) {
      sleep_ms(10);
      continue;
    }

    const ShifterModel::InputState inputs = ShifterInput::readInputs();
    shifterConfig = ShifterInput::configuration();

#ifdef DEBUG
#ifdef DEBUG_ANALOG
    ShifterInput::AnalogState inputAnalog = ShifterInput::readAnalogInput(); //* The button will not work digitally
    Debug::printRawAnalog(inputAnalog);
#endif
    Debug::printRawInputs(inputs);
#endif

    const ShifterModel::ButtonState buttonState = ShifterLogic::resolveButtonState(inputs, shifterConfig);

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
