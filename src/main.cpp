// Firmware para H-shifter (Arduino Leonardo).
// Lê 4 entradas do câmbio e apresenta 6 botões virtuais ao PC como gamepad.
// Versão simplificada: remove leituras dos pedais e foca apenas no H-shifter.
// Código original: DAZ projects.

#include <Joystick.h>

// H-shifter button pin mappings
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

// Estado anterior dos botões (índices 0..BUTTON_COUNT-1)
bool prevButtonState[BUTTON_COUNT] = { 0 };

// GameController: apenas botões (sem eixos analógicos nem hats)
Joystick_ GameController(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
  BUTTON_COUNT, 0,
  false, false, false,
  false, false, false,
  false, false,
  false, false, false);

void setup() {
  Serial.begin(115200);

  GameController.begin();

  pinMode(PIN_BTN4, INPUT_PULLUP);
  pinMode(PIN_BTN5, INPUT_PULLUP);
  pinMode(PIN_BTN6, INPUT_PULLUP);
  pinMode(PIN_BTN7, INPUT_PULLUP);
  pinMode(PIN_BTN8, INPUT_PULLUP);

  pinMode(PIN_BTN9, INPUT_PULLUP);
  pinMode(PIN_BTN10, INPUT_PULLUP);
  pinMode(PIN_BTN11, INPUT_PULLUP);
}

void loop() {
  // Leitura das entradas (LOW = acionado)
  bool btn4 = (digitalRead(PIN_BTN4) == LOW);
  bool btn5 = (digitalRead(PIN_BTN5) == LOW);
  bool btn6 = (digitalRead(PIN_BTN6) == LOW);
  bool btn7 = (digitalRead(PIN_BTN7) == LOW);
  bool btn8 = (digitalRead(PIN_BTN8) == LOW);

  bool btn9 = (digitalRead(PIN_BTN9) == LOW);
  bool btn10 = (digitalRead(PIN_BTN10) == LOW);
  bool btn11 = (digitalRead(PIN_BTN11) == LOW);

  // Calcula novo estado dos botões
  bool newButtonState[BUTTON_COUNT] = { 0 };

  bool comb4Used = false;
  bool comb7Used = false;

  // --- Combinações ---
  if (btn4 && btn5) { newButtonState[0] = true; comb4Used = true; }
  if (btn5 && btn7) { newButtonState[1] = true; comb7Used = true; }
  if (btn4 && btn6) { newButtonState[4] = true; comb4Used = true; }
  if (btn6 && btn7) { newButtonState[5] = true; comb7Used = true; }

  // --- Saídas individuais (apenas se não em combinação) ---
  if (btn4 && !comb4Used) newButtonState[2] = true;
  if (btn7 && !comb7Used) newButtonState[3] = true;

  //TODO Sei que a marcha ré não funciona assim, pois precisa que btn8 esteja sempre pressionado, talvez fazer algo mecânico para resolver isso.
  if (btn8 && comb4Used) newButtonState[6] = true;
  
  // Botões da manopla de caminhão
  if (btn9) newButtonState[7] = true;
  if (btn10) newButtonState[8] = true;
  if (btn11) newButtonState[9] = true;

  // --- Atualiza somente se houver alteração ---
  for (int i = 0; i < BUTTON_COUNT; i++) {
    if (newButtonState[i] != prevButtonState[i]) {
      GameController.setButton(i, newButtonState[i]);
      prevButtonState[i] = newButtonState[i];
    }
  }
  delay(20);
}