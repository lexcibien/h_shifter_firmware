#pragma once

#include <array>
#include <pico/stdlib.h>

class ShifterModel {
public:
  enum ControllerButtons : uint8_t {
    GEAR_1 = 0,
    GEAR_2,
    GEAR_3,
    GEAR_4,
    GEAR_5,
    GEAR_6,
    GEAR_R,
    RANGE,
    SPLIT,
    ENGINE_BRAKE,
    SW_SEQ_PLUS,
    SW_SEQ_MINUS,
    COUNT // Marchas (6 + R), botões sequenciais (2) e botões da manopla (3)
  };

  static constexpr uint8_t BUTTON_COUNT = COUNT;
  static constexpr bool ENABLE_REVERSE = true;
  using ButtonState = std::array<bool, BUTTON_COUNT>;

  struct InputState {
    bool swFront = false;
    bool swLeft = false;
    bool swRight = false;
    bool swBack = false;
    bool swReverse = false;
    bool swRange = false;
    bool swSplit = false;
    bool btnEngineBrake = false;
  };

  struct ShifterConfig {
    bool sequentialEnabled = false;
    bool handleConnected = false;
  };
};
