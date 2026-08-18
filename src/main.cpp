#include <Joystick.h>
#include <array>

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

// Número de botões lógicos reportados pelo Joystick
const uint8_t BUTTON_COUNT = COUNT;
const bool initAutoSendState = true;

bool detectHandleConnection();

bool handleConnected = false;
bool isReverseGear = false;
std::array<bool, BUTTON_COUNT> prevButtonState = {};

bool swEnableReverse;
bool swEnableSequential;

void setup() {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  Serial.begin(115200);

  Joystick.begin();
  Joystick.useManualSend(!initAutoSendState);
  prevButtonState.fill(LOW);

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

  swEnableReverse = (digitalRead(SW_ENABLE_REVERSE) == HIGH);
  swEnableSequential = (digitalRead(SW_ENABLE_SEQUENTIAL) == LOW);

  if (!handleConnected) {
    Joystick.setButton(RANGE, LOW);
    Joystick.setButton(SPLIT, LOW);
    Joystick.setButton(ENGINE_BRAKE, LOW);

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
  bool swFront = (digitalRead(SW_FRONT) == LOW);
  bool swLeft = (digitalRead(SW_LEFT) == LOW);
  bool swRight = (digitalRead(SW_RIGHT) == LOW);
  bool swBack = (digitalRead(SW_BACK) == LOW);
  bool swReverse = (digitalRead(SW_REVERSE) == LOW);

  bool swRange = (digitalRead(SW_KNOB_RANGE) == LOW);
  bool swSplit = (digitalRead(SW_KNOB_SPLIT) == LOW);
  bool btnEngineBrake = (digitalRead(BTN_KNOB_ENGINE_BRAKE) == LOW);

#ifdef DEBUG
  Serial.print("RAW:");
  Serial.print(swFront);
  Serial.print(", ");
  Serial.print(swLeft);
  Serial.print(", ");
  Serial.print(swRight);
  Serial.print(", ");
  Serial.print(swBack);
  Serial.print(", ");
  Serial.print(swReverse);
  Serial.print(", ");

  Serial.print(swRange);
  Serial.print(", ");
  Serial.print(swSplit);
  Serial.print(", ");
  Serial.print(btnEngineBrake);
  Serial.println();
#endif

  std::array<bool, BUTTON_COUNT> newButtonState = {};

  bool combFrontUsed = false;
  bool combBackUsed = false;

  // Combinações para marchas laterais
  if (swBack && swRight && !isReverseGear) {
    newButtonState[GEAR_1] = HIGH;
    combFrontUsed = true;
  }
  if (swFront && swRight) {
    newButtonState[GEAR_2] = HIGH;
    combBackUsed = true;
  }
  if (swBack && swLeft) {
    newButtonState[GEAR_5] = HIGH;
    combFrontUsed = true;
  }
  if (swFront && swLeft) {
    newButtonState[GEAR_6] = HIGH;
    combBackUsed = true;
  }

  if (swEnableReverse && swReverse && newButtonState[GEAR_1]) {
    newButtonState[GEAR_1] = LOW;
    isReverseGear = true;
  }

  if (isReverseGear) {
    newButtonState[GEAR_R] = HIGH;
    combFrontUsed = true;
  }

  // Marchas centrais sequenciais
  if (swEnableSequential) {
    if (swFront && !combFrontUsed) {
      newButtonState[SW_SEQ_MINUS] = HIGH;
    }
    if (swBack && !combBackUsed) {
      newButtonState[SW_SEQ_PLUS] = HIGH;
    }
  }

  // Marchas centrais
  if (swBack && !combFrontUsed) {
    newButtonState[GEAR_3] = HIGH;
  }
  if (swFront && !combBackUsed) {
    newButtonState[GEAR_4] = HIGH;
  }

  if (!swFront && !swLeft && !swRight && !swBack) {
    isReverseGear = false;
  }

  // Botões da manopla de caminhão
  if (handleConnected) {
    newButtonState[RANGE] = swRange;
    newButtonState[SPLIT] = swSplit;
    newButtonState[ENGINE_BRAKE] = btnEngineBrake;
  }

#ifdef DEBUG
  Serial.print("OUT: ");
#endif
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    if (newButtonState.at(i) != prevButtonState.at(i)) {
      Joystick.setButton(i, newButtonState.at(i));
    }
#ifdef DEBUG
    Serial.print(newButtonState.at(i));
#endif
  }
  prevButtonState = newButtonState;
#ifdef DEBUG
  Serial.println();
#endif
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  delay(20);
}

bool detectHandleConnection() {
  int adc = analogRead(SW_KNOB_RANGE);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  return (adc > 300 && adc < 600);
}
