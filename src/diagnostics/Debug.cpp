#if defined(DEBUG) && H_SHIFTER_LOG

#include "Debug.h"
#include <cstdio>
#include <pico/stdio.h>

void Debug::printRawAnalog(const ShifterInput::AnalogState& input) { printf("[DEBUG] analog: adc=%d voltage=%.2fV\n", input.adc, input.voltage); }

void Debug::printRawInputs(const ShifterModel::InputState& inputs) {
  stdio_put_string("[DEBUG] inputs: bits=", -1, false, false);
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  const bool states[] = { inputs.swFront,   inputs.swLeft,  inputs.swRight, inputs.swBack,
                          inputs.swReverse, inputs.swRange, inputs.swSplit, inputs.btnEngineBrake };
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  const bool states[] = {
    inputs.hPatternMode, inputs.sw3, inputs.sw4, inputs.sw5, inputs.sw6, inputs.swRange, inputs.swSplit, inputs.btnEngineBrake
  };
#endif

  for (const bool state : states) {
    putchar(state ? '1' : '0');
  }
  putchar('\n');
}

void Debug::printOutputState(const ShifterModel::ButtonState& buttonState) {
  stdio_put_string("[DEBUG] output: buttons=", -1, false, false);
  for (uint8_t i = 0; i < ShifterModel::BUTTON_COUNT; ++i) {
    putchar(buttonState.at(i) ? '1' : '0');
  }
  putchar('\n');
}

#endif
