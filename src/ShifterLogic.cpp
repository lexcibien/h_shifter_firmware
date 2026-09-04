#include <ShifterLogic.h>

ShifterLogic::ButtonState ShifterLogic::resolveButtonState(const InputState& inputs, const Config& config) {
  ButtonState buttonState = {};
  bool combFrontUsed = false;
  bool combBackUsed = false;

  if (config.sequentialEnabled) {
    resolveSequentialGears(inputs, buttonState, combFrontUsed, combBackUsed);
  } else {
    resolveLateralGears(inputs, buttonState, combFrontUsed, combBackUsed);
    activateReverseGear(inputs, buttonState);
    applyReverseGear(buttonState, combFrontUsed);
    resolveCenterGears(inputs, buttonState, combFrontUsed, combBackUsed);
  }
  resetReverseGear(inputs);
  if (config.handleConnected) {
    applyHandleButtons(inputs, buttonState);
  }
  return buttonState;
}
