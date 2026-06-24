#include <Joystick.h>

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
  COUNT // Marchas (6 + R) e botões da manopla (3)
};

// Número de botões lógicos reportados pelo Joystick
const auto BUTTON_COUNT = static_cast<uint8_t>(ControllerButtons::COUNT); // Marchas (6 + R) e botões da manopla (3)
const bool initAutoSendState = true;

bool detectHandleConnection();

Joystick_ GameController;

bool handleConnected = false;
bool isReverseGear = false;

bool swEnableReverse;
bool swEnableSequential;

void setup() {
  Serial.begin(115200);

  GameController.begin();
  GameController.useManualSend(!initAutoSendState);

  pinMode(SW_FRONT, INPUT_PULLUP);
  pinMode(SW_LEFT, INPUT_PULLUP);
  pinMode(SW_RIGHT, INPUT_PULLUP);
  pinMode(SW_BACK, INPUT_PULLUP);
  pinMode(SW_REVERSE, INPUT_PULLUP);

  pinMode(SW_ENABLE_REVERSE, INPUT_PULLUP);
  pinMode(SW_ENABLE_SEQUENTIAL, INPUT_PULLUP);

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  delay(2000);

  handleConnected = detectHandleConnection();

  swEnableReverse = (digitalRead(SW_ENABLE_REVERSE) == LOW);
  swEnableSequential = (digitalRead(SW_ENABLE_SEQUENTIAL) == LOW);

  if (!handleConnected) {
    GameController.setButton(RANGE, LOW);
    GameController.setButton(SPLIT, LOW);
    GameController.setButton(ENGINE_BRAKE, LOW);

    Serial.println("INFO: Truck shifter handle not connected");
  } else {
    pinMode(SW_KNOB_RANGE, INPUT_PULLUP);
    pinMode(SW_KNOB_SPLIT, INPUT_PULLUP);
    pinMode(BTN_KNOB_ENGINE_BRAKE, INPUT_PULLUP);

    Serial.println("OK: Handle detected");
  }

  Serial.println(swEnableReverse ? "OK: Rear gear is enabled" : "INFO: Rear gear is disabled");
  Serial.println(swEnableSequential ? "OK: The current gear output is sequential" : "INFO: The current gear output is H-Shifter");
}

void loop() {
  static bool prevButtonState[BUTTON_COUNT] = { LOW };

  bool swFront = (digitalRead(SW_FRONT) == LOW);
  bool swLeft = (digitalRead(SW_LEFT) == LOW);
  bool swRight = (digitalRead(SW_RIGHT) == LOW);
  bool swBack = (digitalRead(SW_BACK) == LOW);
  bool swReverse = (digitalRead(SW_REVERSE) == LOW);

  bool swRange = (digitalRead(SW_KNOB_RANGE) == LOW);
  bool swSplit = (digitalRead(SW_KNOB_SPLIT) == LOW);
  bool btnEngineBrake = (digitalRead(BTN_KNOB_ENGINE_BRAKE) == LOW);

  bool newButtonState[BUTTON_COUNT] = { LOW };

  bool combFrontUsed = false;
  bool combBackUsed = false;

  // Combinações para marchas laterais
  if (swFront && swLeft && !isReverseGear) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_1)] = HIGH; combFrontUsed = true; }
  if (swLeft && swBack) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_2)] = HIGH; combBackUsed = true; }
  if (swFront && swRight) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_5)] = HIGH; combFrontUsed = true; }
  if (swRight && swBack) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_6)] = HIGH; combBackUsed = true; }

  // Marchas centrais sequenciais
  if (swEnableSequential) {
    if (swFront && !combFrontUsed) { newButtonState[SW_SEQ_MINUS] = HIGH; }
    if (swBack && !combBackUsed) { newButtonState[SW_SEQ_PLUS] = HIGH; }
  }

  // Marchas centrais
  if (swFront && !combFrontUsed) newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_3)] = HIGH;
  if (swBack && !combBackUsed) newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_4)] = HIGH;

  if (swEnableReverse && swReverse && swLeft && swFront) { newButtonState[GEAR_R] = HIGH; isReverseGear = true; }

  if (!swFront && !swLeft && !swRight && !swBack) { isReverseGear = false; }

  // Botões da manopla de caminhão
  if (handleConnected) {
    if (swRange) newButtonState[static_cast<uint8_t>(ControllerButtons::RANGE)] = HIGH;
    if (swSplit) newButtonState[static_cast<uint8_t>(ControllerButtons::SPLIT)] = HIGH;
    if (btnEngineBrake) newButtonState[static_cast<uint8_t>(ControllerButtons::ENGINE_BRAKE)] = HIGH;
  }

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    if (newButtonState[i] != prevButtonState[i]) {
      GameController.setButton(i, newButtonState[i]);
      prevButtonState[i] = newButtonState[i];
    }
  }
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  delay(20);
}

bool detectHandleConnection() {
  int adc = analogRead(SW_KNOB_RANGE);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  return (adc > 300 && adc < 600);
}
