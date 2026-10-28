#include <Adafruit_TinyUSB.h>
#include <Debug.h>

void Debug::printRawAnalog(const ShifterInput::AnalogState& input) { Serial.printf("RAW AN: %d, %0.2f\n", input.adc, input.voltage); }

void Debug::printRawInputs(const ShifterModel::InputState& inputs) {
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  Serial.printf("RAW: %d, %d, %d, %d, %d, %d, %d, %d \n", inputs.swFront, inputs.swLeft, inputs.swRight, inputs.swBack, inputs.swReverse,
                inputs.swRange, inputs.swSplit, inputs.btnEngineBrake);
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  Serial.printf("RAW: mode=%d, pin3=%d, pin4=%d, pin5=%d, pin6=%d, range=%d, split=%d, engine=%d\n", inputs.hPatternMode, inputs.sw3, inputs.sw4,
                inputs.sw5, inputs.sw6, inputs.swRange, inputs.swSplit, inputs.btnEngineBrake);
#endif
}

void Debug::printOutputState(const ShifterModel::ButtonState& buttonState) {
  Serial.print("OUT: ");
  for (uint8_t i = 0; i < ShifterModel::BUTTON_COUNT; ++i) {
    Serial.print(buttonState.at(i));
  }
  Serial.println();
}
