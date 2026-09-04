#pragma once

#include <ShifterModel.h>

class ShifterLogic {
private:
  using InputState = ShifterModel::InputState;
  using Config = ShifterModel::ShifterConfig;
  using ButtonState = ShifterModel::ButtonState;
  using Buttons = ShifterModel::ControllerButtons;

  inline static bool isReverseGear = false;

  static void resolveLateralGears(const InputState& inputs, ButtonState& buttonState, bool& combFrontUsed, bool& combBackUsed) {
    if (inputs.swBack && inputs.swRight && !isReverseGear) {
      buttonState[Buttons::GEAR_1] = true;
      combFrontUsed = true;
    }
    if (inputs.swFront && inputs.swRight) {
      buttonState[Buttons::GEAR_2] = true;
      combBackUsed = true;
    }
    if (inputs.swBack && inputs.swLeft) {
      buttonState[Buttons::GEAR_5] = true;
      combFrontUsed = true;
    }
    if (inputs.swFront && inputs.swLeft) {
      buttonState[Buttons::GEAR_6] = true;
      combBackUsed = true;
    }
  }

  static void activateReverseGear(const InputState& inputs, ButtonState& buttonState) {
    if (ShifterModel::ENABLE_REVERSE && inputs.swReverse && inputs.swBack && inputs.swRight) {
      buttonState[Buttons::GEAR_1] = false;
      isReverseGear = true;
    }
  }

  static void applyReverseGear(ButtonState& buttonState, bool& combFrontUsed) {
    if (isReverseGear) {
      buttonState[Buttons::GEAR_R] = true;
      combFrontUsed = true;
    }
  }

  static void resolveSequentialGears(const InputState& inputs, ButtonState& buttonState, bool combFrontUsed, bool combBackUsed) {
    if (inputs.swFront && !combFrontUsed) {
      buttonState[Buttons::SW_SEQ_MINUS] = true;
    }
    if (inputs.swBack && !combBackUsed) {
      buttonState[Buttons::SW_SEQ_PLUS] = true;
    }
  }

  static void resolveCenterGears(const InputState& inputs, ButtonState& buttonState, bool combFrontUsed, bool combBackUsed) {
    if (inputs.swBack && !combFrontUsed) {
      buttonState[Buttons::GEAR_3] = true;
    }
    if (inputs.swFront && !combBackUsed) {
      buttonState[Buttons::GEAR_4] = true;
    }
  }

  static void resetReverseGear(const InputState& inputs) {
    if (!inputs.swFront && !inputs.swLeft && !inputs.swRight && !inputs.swBack) {
      isReverseGear = false;
    }
  }

  static void applyHandleButtons(const InputState& inputs, ButtonState& buttonState) {
    buttonState[Buttons::RANGE] = inputs.swRange;
    buttonState[Buttons::SPLIT] = inputs.swSplit;
    buttonState[Buttons::ENGINE_BRAKE] = inputs.btnEngineBrake;
  }

public:
  static ButtonState resolveButtonState(const InputState& inputs, const Config& config);
};
