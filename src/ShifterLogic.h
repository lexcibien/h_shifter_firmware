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

  static void resolveSequentialGears(const InputState& inputs, ButtonState& buttonState) {
    std::get<Buttons::SW_SEQ_MINUS>(buttonState) = inputs.swFront;
    std::get<Buttons::SW_SEQ_PLUS>(buttonState) = inputs.swBack;
  }

  static bool resolveSpecialGears(const InputState& inputs, ButtonState& buttonState) {
    if (ShifterModel::ENABLE_REVERSE && inputs.swReverse && inputs.swBack && inputs.swRight) {
      isReverseGear = true;
    }
    if (isReverseGear) {
      std::get<Buttons::GEAR_R>(buttonState) = true;
      return true;
    }
    return false;
  }

  static bool resolveLateralGears(const InputState& inputs, ButtonState& buttonState) {
    if (inputs.swBack && inputs.swRight) {
      std::get<Buttons::GEAR_1>(buttonState) = true;
    } else if (inputs.swFront && inputs.swRight) {
      std::get<Buttons::GEAR_2>(buttonState) = true;
    } else if (inputs.swBack && inputs.swLeft) {
      std::get<Buttons::GEAR_5>(buttonState) = true;
    } else if (inputs.swFront && inputs.swLeft) {
      std::get<Buttons::GEAR_6>(buttonState) = true;
    } else {
      return false;
    }
    return true;
  }

  static void resolveCenterGears(const InputState& inputs, ButtonState& buttonState) {
    if (inputs.swBack) {
      std::get<Buttons::GEAR_3>(buttonState) = true;
    } else if (inputs.swFront) {
      std::get<Buttons::GEAR_4>(buttonState) = true;
    }
  }

  static void resetReverseGear(const InputState& inputs) {
    if (!inputs.swFront && !inputs.swLeft && !inputs.swRight && !inputs.swBack) {
      isReverseGear = false;
    }
  }

#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  static void resolveSequentialGears(const InputState& inputs, ButtonState& buttonState) {
    std::get<Buttons::SW_SEQ_PLUS>(buttonState) = inputs.sw6;
    std::get<Buttons::SW_SEQ_MINUS>(buttonState) = inputs.sw3;
  }

  static bool resolveSpecialGears(const InputState& inputs, ButtonState& buttonState) {
    if (inputs.sw3 && inputs.sw4 && inputs.sw5) {
      std::get<Buttons::GEAR_7>(buttonState) = true;
    } else if (inputs.sw6 && inputs.sw4 && inputs.sw5) {
      std::get<Buttons::GEAR_R>(buttonState) = true;
    } else {
      return false;
    }
    return true;
  }

  static bool resolveLateralGears(const InputState& inputs, ButtonState& buttonState) {
    if (inputs.sw3 && inputs.sw4) {
      std::get<Buttons::GEAR_1>(buttonState) = true;
    } else if (inputs.sw6 && inputs.sw4) {
      std::get<Buttons::GEAR_2>(buttonState) = true;
    } else if (inputs.sw3 && inputs.sw5) {
      std::get<Buttons::GEAR_5>(buttonState) = true;
    } else if (inputs.sw6 && inputs.sw5) {
      std::get<Buttons::GEAR_6>(buttonState) = true;
    } else {
      return false;
    }
    return true;
  }

  static void resolveCenterGears(const InputState& inputs, ButtonState& buttonState) {
    if (inputs.sw3) {
      std::get<Buttons::GEAR_3>(buttonState) = true;
    } else if (inputs.sw6) {
      std::get<Buttons::GEAR_4>(buttonState) = true;
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
