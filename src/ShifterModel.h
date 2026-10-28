#pragma once

#include <array>
#include <board.h>
#include <pico/stdlib.h>

class ShifterModel {
public:
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
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
    COUNT, // Marchas (6 + R), botões sequenciais (2) e botões da manopla (3)
  };
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  enum ControllerButtons : uint8_t {
    GEAR_1 = 0,
    GEAR_2,
    GEAR_3,
    GEAR_4,
    GEAR_5,
    GEAR_6,
    GEAR_7,
    GEAR_R,
    SW_SEQ_PLUS,
    SW_SEQ_MINUS,
    RANGE,
    SPLIT,
    ENGINE_BRAKE,
    COUNT, // Marchas (7 + R), botoes sequenciais (2) e botoes da manopla (3)
  };
#endif

  static constexpr uint8_t BUTTON_COUNT = COUNT;
  static constexpr bool ENABLE_REVERSE = true;
  using ButtonState = std::array<bool, BUTTON_COUNT>;

  struct InputState {
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
    bool swFront = false;
    bool swLeft = false;
    bool swRight = false;
    bool swBack = false;
    bool swReverse = false;
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
    bool hPatternMode = false;
    bool sw3 = false;
    bool sw4 = false;
    bool sw5 = false;
    bool sw6 = false;
#endif
    bool swRange = false;
    bool swSplit = false;
    bool btnEngineBrake = false;
  };

  struct ShifterConfig {
    bool sequentialEnabled = false;
    bool handleConnected = false;
  };
};
