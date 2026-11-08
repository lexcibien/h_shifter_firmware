#if defined(H_SHIFTER_DEBUG) && H_SHIFTER_LOG

#include "Debug.h"
#include <Adafruit_USBD_CDC.h>

void Debug::printRawAnalog(const ShifterInput::AnalogState& input) { Serial.printf("[DEBUG] analog: adc=%d voltage=%.2fV\n", input.adc, input.voltage); }

void Debug::printRawInputs(const ShifterModel::InputState& inputs) {
  Serial.print("[DEBUG] inputs: bits=");
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  const bool states[] = { inputs.swFront,   inputs.swLeft,  inputs.swRight, inputs.swBack,
                          inputs.swReverse, inputs.swRange, inputs.swSplit, inputs.btnEngineBrake };
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  const bool states[] = {
    inputs.hPatternMode, inputs.sw3, inputs.sw4, inputs.sw5, inputs.sw6, inputs.swRange, inputs.swSplit, inputs.btnEngineBrake
  };
#endif

  for (const bool state : states) {
    Serial.write(state ? '1' : '0');
  }
  Serial.println();
}

void Debug::printOutputState(const ShifterModel::ButtonState& buttonState) {
  Serial.print("[DEBUG] output: buttons=");
  for (uint8_t i = 0; i < ShifterModel::BUTTON_COUNT; ++i) {
    Serial.write(buttonState.at(i) ? '1' : '0');
  }
  Serial.println();
}

#endif
