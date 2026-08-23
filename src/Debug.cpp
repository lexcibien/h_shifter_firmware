#include <Debug.h>

void Debug::printRawInputs(const ShifterModel::InputState& inputs) {
  Serial.print("RAW:");
  Serial.print(inputs.swFront);
  Serial.print(", ");
  Serial.print(inputs.swLeft);
  Serial.print(", ");
  Serial.print(inputs.swRight);
  Serial.print(", ");
  Serial.print(inputs.swBack);
  Serial.print(", ");
  Serial.print(inputs.swReverse);
  Serial.print(", ");
  Serial.print(inputs.swRange);
  Serial.print(", ");
  Serial.print(inputs.swSplit);
  Serial.print(", ");
  Serial.print(inputs.btnEngineBrake);
  Serial.println();
}

void Debug::printOutputState(const ShifterModel::ButtonState& buttonState) {
  Serial.print("OUT: ");
  for (uint8_t i = 0; i < ShifterModel::BUTTON_COUNT; ++i) {
    Serial.print(buttonState.at(i));
  }
  Serial.println();
}
