// Firmware para H-shifter (Arduino Leonardo).
// Lê 4 entradas do câmbio e apresenta 6 botões virtuais ao PC como gamepad.
// Versão simplificada: remove leituras dos pedais e foca apenas no H-shifter.
// Código original: DAZ projects.

#include "../boards/pinouts/pins_arduino.h"
#include "Arduino.h"
#include <Joystick.h>

enum class ControllerButtons : int {
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
  BUTTON_COUNT // Marchas (6 + R) e botões da manopla (3)
};

// Número de botões lógicos reportados pelo Joystick
const auto BUTTON_COUNT = static_cast<uint8_t>(ControllerButtons::BUTTON_COUNT); // Marchas (6 + R) e botões da manopla (3)
const bool initAutoSendState = true;

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

void setup() {
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

  delay(2000);

  int adc = analogRead(SW_RANGE);
  handleConnected = (adc > 300 && adc < 600);

  if (!handleConnected)
    Serial.println("WARN: Truck shifter handle not connected");
  else
    Serial.println("OK: Handle detected");

  if (handleConnected) {
    pinMode(SW_RANGE, INPUT_PULLUP);
    pinMode(SW_SPLIT, INPUT_PULLUP);
    pinMode(BTN_ENGINE_BRAKE, INPUT_PULLUP);
  }

  if (!handleConnected) {
    GameController.setButton(static_cast<uint8_t>(ControllerButtons::SW_RANGE), LOW);
    GameController.setButton(static_cast<uint8_t>(ControllerButtons::SW_SPLIT), LOW);
    GameController.setButton(static_cast<uint8_t>(ControllerButtons::BTN_ENGINE_BRAKE), LOW);
  }
}

void loop() {
  static bool prevButtonState[BUTTON_COUNT] = { LOW };

  bool swFront = (digitalRead(SW_FRONT) == LOW);
  bool swLeft = (digitalRead(SW_LEFT) == LOW);
  bool swRight = (digitalRead(SW_RIGHT) == LOW);
  bool swBack = (digitalRead(SW_BACK) == LOW);
  bool swReverse = (digitalRead(SW_REVERSE) == LOW);

  bool swRange = (digitalRead(SW_RANGE) == LOW);
  bool swSplit = (digitalRead(SW_SPLIT) == LOW);
  bool btnEngineBrake = (digitalRead(BTN_ENGINE_BRAKE) == LOW);

  bool newButtonState[BUTTON_COUNT] = { LOW };

  bool combFrontUsed = false;
  bool combBackUsed = false;

  // Combinações para marchas laterais
  if (swFront && swLeft) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_1)] = HIGH; combFrontUsed = true; }
  if (swLeft && swBack) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_2)] = HIGH; combBackUsed = true; }
  if (swFront && swRight) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_5)] = HIGH; combFrontUsed = true; }
  if (swRight && swBack) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_6)] = HIGH; combBackUsed = true; }

  // Marchas centrais
  if (swFront && !combFrontUsed) newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_3)] = HIGH;
  if (swBack && !combBackUsed) newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_4)] = HIGH;

  //* Sei que a marcha ré não funciona assim, pois precisa que swReverse esteja sempre pressionado, talvez fazer algo mecânico para resolver isso.
  if (swReverse && combFrontUsed) newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_R)] = HIGH;

  // Botões da manopla de caminhão
  if (handleConnected) {
    if (swRange) newButtonState[static_cast<uint8_t>(ControllerButtons::SW_RANGE)] = HIGH;
    if (swSplit) newButtonState[static_cast<uint8_t>(ControllerButtons::SW_SPLIT)] = HIGH;
    if (btnEngineBrake) newButtonState[static_cast<uint8_t>(ControllerButtons::BTN_ENGINE_BRAKE)] = HIGH;
  }

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    if (newButtonState[i] != prevButtonState[i]) {
      GameController.setButton(i, newButtonState[i]);
      prevButtonState[i] = newButtonState[i];
    }
  }
  delay(20);
}
