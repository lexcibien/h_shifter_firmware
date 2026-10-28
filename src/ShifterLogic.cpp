#include <ShifterLogic.h>

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
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
#endif

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2

ShifterLogic::ButtonState ShifterLogic::resolveButtonState(const InputState& inputs, const Config& config) {
  ButtonState buttonState = {};

  if (!inputs.hPatternMode) {
    buttonState[Buttons::SW_SEQ_PLUS] = inputs.sw6;
    buttonState[Buttons::SW_SEQ_MINUS] = inputs.sw3;
  } else {
    // As combinacoes com mais chaves precisam vir primeiro.
    if (inputs.sw3 && inputs.sw4 && inputs.sw5) {
      buttonState[Buttons::GEAR_7] = true;
    } else if (inputs.sw6 && inputs.sw4 && inputs.sw5) {
      buttonState[Buttons::GEAR_R] = true;
    } else if (inputs.sw3 && inputs.sw4) {
      buttonState[Buttons::GEAR_1] = true;
    } else if (inputs.sw6 && inputs.sw4) {
      buttonState[Buttons::GEAR_2] = true;
    } else if (inputs.sw3 && inputs.sw5) {
      buttonState[Buttons::GEAR_5] = true;
    } else if (inputs.sw6 && inputs.sw5) {
      buttonState[Buttons::GEAR_6] = true;
    } else if (inputs.sw3) {
      buttonState[Buttons::GEAR_3] = true;
    } else if (inputs.sw6) {
      buttonState[Buttons::GEAR_4] = true;
    }
  }

  if (config.handleConnected) {
    applyHandleButtons(inputs, buttonState);
  }

  return buttonState;
}

#endif
