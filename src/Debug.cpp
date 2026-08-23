#include <Debug.h>
#include <cstdio>

void Debug::printRawInputs(const ShifterModel::InputState& inputs) {
  printf("RAW: %d, %d, %d, %d, %d, %d, %d, %d \n", inputs.swFront, inputs.swLeft, inputs.swRight, inputs.swBack, inputs.swReverse, inputs.swRange,
         inputs.swSplit, inputs.btnEngineBrake);
}

void Debug::printOutputState(const ShifterModel::ButtonState& buttonState) {
  printf("OUT: ");
  for (uint8_t i = 0; i < ShifterModel::BUTTON_COUNT; ++i) {
    printf("%d", buttonState.at(i));
  }
  printf("\n");
}
