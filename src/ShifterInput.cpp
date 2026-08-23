#include <ShifterInput.h>

void ShifterInput::begin() {
    pinMode(SW_FRONT, INPUT_PULLUP);
    pinMode(SW_LEFT, INPUT_PULLUP);
    pinMode(SW_RIGHT, INPUT_PULLUP);
    pinMode(SW_BACK, INPUT_PULLUP);
    pinMode(SW_REVERSE, INPUT_PULLUP);
    pinMode(SW_ENABLE_SEQUENTIAL, INPUT_PULLUP);

    delay(2000);
    handleConnected = detectHandleConnection();

    if (handleConnected) {
      pinMode(SW_KNOB_RANGE, INPUT_PULLUP);
      pinMode(SW_KNOB_SPLIT, INPUT_PULLUP);
      pinMode(BTN_KNOB_ENGINE_BRAKE, INPUT_PULLUP);
    }
  }

  [[nodiscard]] ShifterModel::InputState ShifterInput::readInputs() const {
    ShifterModel::InputState inputs;
    inputs.swFront = digitalRead(SW_FRONT) == LOW;
    inputs.swLeft = digitalRead(SW_LEFT) == LOW;
    inputs.swRight = digitalRead(SW_RIGHT) == LOW;
    inputs.swBack = digitalRead(SW_BACK) == LOW;
    inputs.swReverse = digitalRead(SW_REVERSE) == LOW;

    if (handleConnected) {
      inputs.swRange = digitalRead(SW_KNOB_RANGE) == LOW;
      inputs.swSplit = digitalRead(SW_KNOB_SPLIT) == LOW;
      inputs.btnEngineBrake = digitalRead(BTN_KNOB_ENGINE_BRAKE) == LOW;
    }

    return inputs;
  }

  [[nodiscard]] ShifterModel::ShifterConfig ShifterInput::configuration() const {
    return { .sequentialEnabled = digitalRead(SW_ENABLE_SEQUENTIAL) == LOW, .handleConnected = handleConnected };
  }
