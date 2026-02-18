#include "../boards/pinouts/pins_arduino.h"
#include "Arduino.h"
#include <Joystick.h>

enum ControllerButtons {
  GEAR_1 = 0,
  GEAR_2,
  GEAR_3,
  GEAR_4,
  GEAR_5,
  GEAR_6,
  GEAR_R,
  SW_RANGE,
  SW_SPLIT,
  BTN_ENGINE_BRAKE,
  SW_SEQ_PLUS,
  SW_SEQ_MINUS,
  BUTTON_COUNT // Marchas (6 + R) e botões da manopla (3)
};

// Número de botões lógicos reportados pelo Joystick
const uint8_t BUTTON_COUNT = ControllerButtons::BUTTON_COUNT; // Marchas (6 + R), sequenciais e botões da manopla (3)
const bool initAutoSendState = true;

bool detectHandleConnection();

__attribute__((noreturn)) void setup() {
#if ARDUINO_AVR_LEONARDO
  Joystick_ GameController(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
    BUTTON_COUNT, 0,
    false, false, false,
    false, false, false,
    false, false,
    false, false, false);
#elif ARDUINO_RASPBERRY_PI_PICO_2
  Joystick_ GameController;
#endif

  bool handleConnected = false;
  bool isReverseGear = false;

  Serial.begin(115200);

#if ARDUINO_AVR_LEONARDO
  GameController.begin(initAutoSendState);
#elif ARDUINO_RASPBERRY_PI_PICO_2
  GameController.begin();
  GameController.useManualSend(!initAutoSendState);
#endif

  pinMode(SW_FRONT, INPUT_PULLUP);
  pinMode(SW_LEFT, INPUT_PULLUP);
  pinMode(SW_RIGHT, INPUT_PULLUP);
  pinMode(SW_BACK, INPUT_PULLUP);
  pinMode(SW_REVERSE, INPUT_PULLUP);

  pinMode(SW_ENABLE_REVERSE, INPUT_PULLUP);
  pinMode(SW_ENABLE_SEQUENTIAL, INPUT_PULLUP);

  delay(2000);

  handleConnected = detectHandleConnection();

  bool swEnableReverse = (digitalRead(SW_ENABLE_REVERSE) == LOW);
  bool swEnableSequential = (digitalRead(SW_ENABLE_SEQUENTIAL) == LOW);

  if (!handleConnected) {
    GameController.setButton(SW_RANGE, LOW);
    GameController.setButton(SW_SPLIT, LOW);
    GameController.setButton(BTN_ENGINE_BRAKE, LOW);

    Serial.println("INFO: Truck shifter handle not connected");
  } else {
    pinMode(SW_KNOB_RANGE, INPUT_PULLUP);
    pinMode(SW_KNOB_SPLIT, INPUT_PULLUP);
    pinMode(BTN_KNOB_ENGINE_BRAKE, INPUT_PULLUP);

    Serial.println("OK: Handle detected");
  }

  Serial.println(swEnableReverse ? "OK: Rear gear is enabled" : "INFO: Rear gear is disabled");
  Serial.println(swEnableSequential ? "OK: The current gear output is sequential" : "INFO: The current gear output is H-Shifter");

  while (true) {
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
    if (swFront && swLeft && !isReverseGear) { newButtonState[GEAR_1] = HIGH; combFrontUsed = true; }
    if (swLeft && swBack) { newButtonState[GEAR_2] = HIGH; combBackUsed = true; }
    if (swFront && swRight) { newButtonState[GEAR_5] = HIGH; combFrontUsed = true; }
    if (swRight && swBack) { newButtonState[GEAR_6] = HIGH; combBackUsed = true; }

    // Marchas centrais sequenciais
    if (swEnableSequential) {
      if (swFront && !combFrontUsed) { newButtonState[SW_SEQ_MINUS] = HIGH; }
      if (swBack && !combBackUsed) { newButtonState[SW_SEQ_PLUS] = HIGH; }
    }

    // Marchas centrais
    if (swFront && !combFrontUsed) newButtonState[GEAR_3] = HIGH;
    if (swBack && !combBackUsed) newButtonState[GEAR_4] = HIGH;

    if (swEnableReverse && swReverse && swLeft && swFront) { newButtonState[GEAR_R] = HIGH; isReverseGear = true; }

    if (!swFront && !swLeft && !swRight && !swBack) { isReverseGear = false; }

    // Botões da manopla de caminhão
    if (handleConnected) {
      if (swRange) newButtonState[SW_RANGE] = HIGH;
      if (swSplit) newButtonState[SW_SPLIT] = HIGH;
      if (btnEngineBrake) newButtonState[BTN_ENGINE_BRAKE] = HIGH;
    }

    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
      if (newButtonState[i] != prevButtonState[i]) {
        GameController.setButton(i, newButtonState[i]);
        prevButtonState[i] = newButtonState[i];
      }
    }
    delay(20);
  }
}

void loop() {/* Unused */ }

bool detectHandleConnection() {
  int adc = analogRead(SW_KNOB_RANGE);
  return (adc > 300 && adc < 600);
}
