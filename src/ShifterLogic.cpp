#include <ShifterLogic.h>

ShifterLogic::ButtonState ShifterLogic::resolveButtonState(const InputState& inputs, const Config& config) {
  ButtonState buttonState = {};
  bool combFrontUsed = false;
  bool combBackUsed = false;

  resolveLateralGears(inputs, buttonState, combFrontUsed, combBackUsed);
  activateReverseGear(inputs, buttonState);
  applyReverseGear(buttonState, combFrontUsed);
  resolveSequentialGears(inputs, config, buttonState, combFrontUsed, combBackUsed);
  resolveCenterGears(inputs, buttonState, combFrontUsed, combBackUsed);
  resetReverseGear(inputs);
  applyHandleButtons(inputs, config, buttonState);

  return buttonState;
}
