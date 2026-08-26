#include <Debug.h>

void Debug::printRawInputs(const ShifterModel::InputState& inputs) {
  Serial.printf("RAW: %d, %d, %d, %d, %d, %d, %d, %d \n", inputs.swFront, inputs.swLeft, inputs.swRight, inputs.swBack, inputs.swReverse,
                inputs.swRange, inputs.swSplit, inputs.btnEngineBrake);
}

void Debug::printOutputState(const ShifterModel::ButtonState& buttonState) {
  Serial.print("OUT: ");
  for (uint8_t i = 0; i < ShifterModel::BUTTON_COUNT; ++i) {
    Serial.print(buttonState.at(i));
  }
  Serial.println();
}
