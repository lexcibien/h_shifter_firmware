#include <ShifterLogic.h>

ShifterLogic::ButtonState ShifterLogic::resolveButtonState(const InputState& inputs, const Config& config) {
  ButtonState buttonState = {};

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  resetReverseGear(inputs);
  const bool sequentialMode = config.sequentialEnabled;
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  const bool sequentialMode = !inputs.hPatternMode;
#endif

  if (sequentialMode) {
    resolveSequentialGears(inputs, buttonState);
  } else {
    // As combinações com mais chaves precisam vir primeiro.
    if (!resolveSpecialGears(inputs, buttonState) && !resolveLateralGears(inputs, buttonState)) {
      resolveCenterGears(inputs, buttonState);
    }
  }

  if (config.handleConnected) {
    applyHandleButtons(inputs, buttonState);
  }

  return buttonState;
}
