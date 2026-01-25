// Firmware para H-shifter (Arduino Leonardo).
// Lê 4 entradas do câmbio e apresenta 6 botões virtuais ao PC como gamepad.
// Versão simplificada: remove leituras dos pedais e foca apenas no H-shifter.
// Código original: DAZ projects.

#include <Joystick.h>

enum class ControllerButtons : int {
  GEAR_1,
  GEAR_2,
  GEAR_3,
  GEAR_4,
  GEAR_5,
  GEAR_6,
  GEAR_R,
  SW_SPLIT,
  SW_RANGE,
  BTN_ENGINE_BRAKE
};

// Botões do H-shifter 
const uint8_t PIN_BTN4 = 4;
const uint8_t PIN_BTN5 = 5;
const uint8_t PIN_BTN6 = 6;
const uint8_t PIN_BTN7 = 7;
const uint8_t PIN_BTN8 = 8; // Botão de marcha ré

// Botões da manopla de caminhão
const uint8_t PIN_BTN9 = 9;
const uint8_t PIN_BTN10 = 10;
const uint8_t PIN_BTN11 = 11;

// Número de botões lógicos reportados pelo Joystick
const uint8_t BUTTON_COUNT = 10; // Marchas (6 + R) e botões da manopla (3)
const bool initAutoSendState = true;

Joystick_ GameController(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
  BUTTON_COUNT, 0,
  false, false, false,
  false, false, false,
  false, false,
  false, false, false);

bool handleConnected = false;

void setup() {
  Serial.begin(115200);

  GameController.begin(initAutoSendState);

  pinMode(PIN_BTN4, INPUT_PULLUP);
  pinMode(PIN_BTN5, INPUT_PULLUP);
  pinMode(PIN_BTN6, INPUT_PULLUP);
  pinMode(PIN_BTN7, INPUT_PULLUP);
  pinMode(PIN_BTN8, INPUT_PULLUP);

  delay(2000);

  int adc = analogRead(PIN_BTN9);
  handleConnected = (adc > 300 && adc < 600);

  if (!handleConnected)
    Serial.println("WARN: Truck shifter handle not connected");
  else
    Serial.println("OK: Handle detected");

  if (handleConnected) {
    pinMode(PIN_BTN9, INPUT_PULLUP);
    pinMode(PIN_BTN10, INPUT_PULLUP);
    pinMode(PIN_BTN11, INPUT_PULLUP);
  }

  if (!handleConnected) {
    GameController.setButton(static_cast<uint8_t>(ControllerButtons::SW_SPLIT), 0);
    GameController.setButton(static_cast<uint8_t>(ControllerButtons::SW_RANGE), 0);
    GameController.setButton(static_cast<uint8_t>(ControllerButtons::BTN_ENGINE_BRAKE), 0);
  }
}

void loop() {
  static bool prevButtonState[BUTTON_COUNT] = { false };

  bool btn4 = (digitalRead(PIN_BTN4) == LOW);
  bool btn5 = (digitalRead(PIN_BTN5) == LOW);
  bool btn6 = (digitalRead(PIN_BTN6) == LOW);
  bool btn7 = (digitalRead(PIN_BTN7) == LOW);
  bool btn8 = (digitalRead(PIN_BTN8) == LOW);

  bool btn9 = (digitalRead(PIN_BTN9) == LOW);
  bool btn10 = (digitalRead(PIN_BTN10) == LOW);
  bool btn11 = (digitalRead(PIN_BTN11) == LOW);

  bool newButtonState[BUTTON_COUNT] = { false };

  bool comb4Used = false;
  bool comb7Used = false;

  // Combinações para marchas laterais
  if (btn4 && btn5) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_1)] = true; comb4Used = true; }
  if (btn5 && btn7) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_2)] = true; comb7Used = true; }
  if (btn4 && btn6) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_5)] = true; comb4Used = true; }
  if (btn6 && btn7) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_6)] = true; comb7Used = true; }

  // Marchas centrais
  if (btn4 && !comb4Used) newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_3)] = true;
  if (btn7 && !comb7Used) newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_4)] = true;

  //TODO Sei que a marcha ré não funciona assim, pois precisa que btn8 esteja sempre pressionado, talvez fazer algo mecânico para resolver isso.
  if (btn8 && comb4Used) newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_R)] = true;

  // Botões da manopla de caminhão
  if (handleConnected) {
    if (btn9) newButtonState[static_cast<uint8_t>(ControllerButtons::SW_SPLIT)] = true;
    if (btn10) newButtonState[static_cast<uint8_t>(ControllerButtons::SW_RANGE)] = true;
    if (btn11) newButtonState[static_cast<uint8_t>(ControllerButtons::BTN_ENGINE_BRAKE)] = true;
  }

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    if (newButtonState[i] != prevButtonState[i]) {
      GameController.setButton(i, newButtonState[i]);
      prevButtonState[i] = newButtonState[i];
    }
  }
  delay(20);
}