#pragma once

#include <ShifterModel.h>

class ShifterLogic {
private:
  using InputState = ShifterModel::InputState;
  using Config = ShifterModel::ShifterConfig;
  using ButtonState = ShifterModel::ButtonState;
  using Buttons = ShifterModel::ControllerButtons;

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  inline static bool isReverseGear = false;

  static void resolveLateralGears(const InputState& inputs, ButtonState& buttonState, bool& combFrontUsed, bool& combBackUsed) {
    if (inputs.swBack && inputs.swRight && !isReverseGear) {
      combFrontUsed = true;
      std::get<Buttons::GEAR_1>(buttonState) = true;
    }
    if (inputs.swFront && inputs.swRight) {
      std::get<Buttons::GEAR_2>(buttonState) = true;
      combBackUsed = true;
    }
    if (inputs.swBack && inputs.swLeft) {
      std::get<Buttons::GEAR_5>(buttonState) = true;
      combFrontUsed = true;
    }
    if (inputs.swFront && inputs.swLeft) {
      std::get<Buttons::GEAR_6>(buttonState) = true;
      combBackUsed = true;
    }
  }

  static void activateReverseGear(const InputState& inputs, ButtonState& buttonState) {
    if (ShifterModel::ENABLE_REVERSE && inputs.swReverse && inputs.swBack && inputs.swRight) {
      std::get<Buttons::GEAR_1>(buttonState) = false;
      isReverseGear = true;
    }
  }

  static void applyReverseGear(ButtonState& buttonState, bool& combFrontUsed) {
    if (isReverseGear) {
      std::get<Buttons::GEAR_R>(buttonState) = true;
      combFrontUsed = true;
    }
  }

  static void resolveSequentialGears(const InputState& inputs, ButtonState& buttonState, bool combFrontUsed, bool combBackUsed) {
    if (inputs.swFront && !combFrontUsed) {
      std::get<Buttons::SW_SEQ_MINUS>(buttonState) = true;
    }
    if (inputs.swBack && !combBackUsed) {
      std::get<Buttons::SW_SEQ_PLUS>(buttonState) = true;
    }
  }

  static void resolveCenterGears(const InputState& inputs, ButtonState& buttonState, bool combFrontUsed, bool combBackUsed) {
    if (inputs.swBack && !combFrontUsed) {
      std::get<Buttons::GEAR_3>(buttonState) = true;
    }
    if (inputs.swFront && !combBackUsed) {
      std::get<Buttons::GEAR_4>(buttonState) = true;
    }
  }

  static void resetReverseGear(const InputState& inputs) {
    if (!inputs.swFront && !inputs.swLeft && !inputs.swRight && !inputs.swBack) {
      isReverseGear = false;
    }
  }
#endif

  static void applyHandleButtons(const InputState& inputs, ButtonState& buttonState) {
    std::get<Buttons::RANGE>(buttonState) = inputs.swRange;
    std::get<Buttons::SPLIT>(buttonState) = inputs.swSplit;
    std::get<Buttons::ENGINE_BRAKE>(buttonState) = inputs.btnEngineBrake;
  }

public:
  static ButtonState resolveButtonState(const InputState& inputs, const Config& config);
};
