#include "../boards/pinouts/pins_arduino.h"
#include "Arduino.h"
#include <Joystick.h>

enum class ControllerButtons : int {
  GEAR_1 = 0,
  GEAR_2,
  GEAR_3,
  GEAR_4,
  GEAR_5,
  GEAR_R,
  BUTTON_COUNT // Marchas (5 + R)
};

// Número de botões lógicos reportados pelo Joystick
const auto BUTTON_COUNT = static_cast<uint8_t>(ControllerButtons::BUTTON_COUNT);
const bool initAutoSendState = true;

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

  delay(2000);

  while (true) {
    static bool prevButtonState[BUTTON_COUNT] = { LOW };

    bool swFront = (digitalRead(SW_FRONT) == LOW);
    bool swLeft = (digitalRead(SW_LEFT) == LOW);
    bool swRight = (digitalRead(SW_RIGHT) == LOW);
    bool swBack = (digitalRead(SW_BACK) == LOW);

    bool newButtonState[BUTTON_COUNT] = { LOW };

    bool combFrontUsed = false;
    bool combBackUsed = false;

    // Combinações para marchas laterais
    if (swFront && swLeft) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_1)] = HIGH; combFrontUsed = true; }
    if (swLeft && swBack) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_2)] = HIGH; combBackUsed = true; }
    if (swFront && swRight) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_5)] = HIGH; combFrontUsed = true; }
    if (swRight && swBack) { newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_R)] = HIGH; combBackUsed = true; }

    // Marchas centrais
    if (swFront && !combFrontUsed) newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_3)] = HIGH;
    if (swBack && !combBackUsed) newButtonState[static_cast<uint8_t>(ControllerButtons::GEAR_4)] = HIGH;

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
