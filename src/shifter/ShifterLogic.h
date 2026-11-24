#pragma once

#include "ShifterModel.h"

class ShifterLogic : public ShifterModel {
private:
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  inline static bool isReverseGear = false;

  static void resolveSequentialGears(const InputState& inputs, ButtonState& buttonState) {
    std::get<SW_SEQ_MINUS>(buttonState) = inputs.swFront;
    std::get<SW_SEQ_PLUS>(buttonState) = inputs.swBack;
  }

  static bool resolveSpecialGears(const InputState& inputs, const ShifterConfig& config, ButtonState& buttonState) {
    if (inputs.swReverse && inputs.swBack && inputs.swRight) {
      if (config.reverseEnabled) {
        isReverseGear = true;
        std::get<GEAR_R>(buttonState) = true;
      }
      return true;
    }
    if (isReverseGear) {
      std::get<GEAR_R>(buttonState) = config.reverseEnabled;
      return true;
    }
    return false;
  }

  static bool resolveLateralGears(const InputState& inputs, ButtonState& buttonState) {
    if (inputs.swBack && inputs.swRight) {
      std::get<GEAR_1>(buttonState) = true;
    } else if (inputs.swFront && inputs.swRight) {
      std::get<GEAR_2>(buttonState) = true;
    } else if (inputs.swBack && inputs.swLeft) {
      std::get<GEAR_5>(buttonState) = true;
    } else if (inputs.swFront && inputs.swLeft) {
      std::get<GEAR_6>(buttonState) = true;
    } else {
      return false;
    }
    return true;
  }

  static void resolveCenterGears(const InputState& inputs, ButtonState& buttonState) {
    if (inputs.swBack) {
      std::get<GEAR_3>(buttonState) = true;
    } else if (inputs.swFront) {
      std::get<GEAR_4>(buttonState) = true;
    }
  }

  static void resetReverseGear(const InputState& inputs) {
    if (!inputs.swFront && !inputs.swLeft && !inputs.swRight && !inputs.swBack) {
      isReverseGear = false;
    }
  }

#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  static void resolveSequentialGears(const InputState& inputs, ButtonState& buttonState) {
    std::get<SW_SEQ_PLUS>(buttonState) = inputs.sw6;
    std::get<SW_SEQ_MINUS>(buttonState) = inputs.sw3;
  }

  static bool resolveSpecialGears(const InputState& inputs, const ShifterConfig& config, ButtonState& buttonState) {
    if (inputs.sw3 && inputs.sw4 && inputs.sw5) {
      std::get<GEAR_7>(buttonState) = config.forwardGearCount >= 7;
    } else if (inputs.sw6 && inputs.sw4 && inputs.sw5) {
      std::get<GEAR_R>(buttonState) = config.reverseEnabled;
    } else {
      return false;
    }
    return true;
  }

  static bool resolveLateralGears(const InputState& inputs, ButtonState& buttonState) {
    if (inputs.sw3 && inputs.sw4) {
      std::get<GEAR_1>(buttonState) = true;
    } else if (inputs.sw6 && inputs.sw4) {
      std::get<GEAR_2>(buttonState) = true;
    } else if (inputs.sw3 && inputs.sw5) {
      std::get<GEAR_5>(buttonState) = true;
    } else if (inputs.sw6 && inputs.sw5) {
      std::get<GEAR_6>(buttonState) = true;
    } else {
      return false;
    }
    return true;
  }

  static void resolveCenterGears(const InputState& inputs, ButtonState& buttonState) {
    if (inputs.sw3) {
      std::get<GEAR_3>(buttonState) = true;
    } else if (inputs.sw6) {
      std::get<GEAR_4>(buttonState) = true;
    }
  }
#endif

  static void applyHandleButtons(const InputState& inputs, ButtonState& buttonState) {
    std::get<RANGE>(buttonState) = inputs.swRange;
    std::get<SPLIT>(buttonState) = inputs.swSplit;
    std::get<ENGINE_BRAKE>(buttonState) = inputs.btnEngineBrake;
  }

  static void applyGearLimit(const ShifterConfig& config, ButtonState& buttonState) {
    if (config.forwardGearCount < 5) {
      std::get<GEAR_5>(buttonState) = false;
    }
    if (config.forwardGearCount < 6) {
      std::get<GEAR_6>(buttonState) = false;
    }
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
    if (config.forwardGearCount < 7) {
      std::get<GEAR_7>(buttonState) = false;
    }
#endif
    if (!config.reverseEnabled) {
      std::get<GEAR_R>(buttonState) = false;
    }
  }

public:
  static ButtonState resolveButtonState(const InputState& inputs, const ShifterConfig& config);
};
