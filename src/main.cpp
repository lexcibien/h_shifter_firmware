//simple script for using the arduino leonardo board to read the input from the loadcell, hall sensors and 4 buttons, and present them to the PC as a joystick input.
//code written and tested by DAZ projects for use with the DAZ racing pedal set and H shifter.

#include <Joystick.h>
#include <HX711.h>

#define PIN_DATA 3
#define PIN_CLOCK 2

bool buttonState[6] = {0,0,0,0,0,0};

#define LOADCELL_SCALE 1000   // Adjust this value to calibrate brake sensitivity
HX711 brakeSensor;

Joystick_ GameController(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
  6, 0,                  
  false, false, false,   
  true, true, true,      
  false, false,          
  false, false, false);  

int throttleVal = 0;
int clutchVal = 0;
int brakeVal = 0;
int prevBrakeVal = 0;

void setup() {
  Serial.begin(38400);
  GameController.begin();
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  GameController.setRyAxisRange(0, 1023);
  brakeSensor.begin(PIN_DATA, PIN_CLOCK);
  brakeSensor.set_scale(LOADCELL_SCALE);
  brakeSensor.tare();
}

void loop() {

  throttleVal = analogRead(A0);
  throttleVal = map(throttleVal, 0, 32768, 0, 32768);
  GameController.setRxAxis(throttleVal);

  clutchVal = analogRead(A1);
  clutchVal = map(clutchVal, 0, 32768, 0, 32768);
  GameController.setRzAxis(clutchVal);

  // Brake
  int rawBrake = brakeSensor.get_units();
  Serial.println(rawBrake);

  if (rawBrake < 1) {
    brakeVal = 0;
  } else {
    brakeVal = rawBrake;
  }

  if (brakeVal != prevBrakeVal) {
    GameController.setRyAxis(brakeVal);
    prevBrakeVal = brakeVal;
  }
  bool B4 = (digitalRead(4) == LOW);
  bool B5 = (digitalRead(5) == LOW);
  bool B6 = (digitalRead(6) == LOW);
  bool B7 = (digitalRead(7) == LOW);

  // Calcolo nuovo stato dei pulsanti
  bool newState[6] = {0,0,0,0,0,0};

  bool comb4_used = false;
  bool comb7_used = false;

  // --- combinazioni ---
  if (B4 && B5) { newState[0] = 1; comb4_used = true; }
  if (B5 && B7) { newState[1] = 1; comb7_used = true; }
  if (B4 && B6) { newState[4] = 1; comb4_used = true; }
  if (B6 && B7) { newState[5] = 1; comb7_used = true; }

  // --- uscite singole (solo se non in combinazione) ---
  if (B4 && !comb4_used) newState[2] = 1;
  if (B7 && !comb7_used) newState[3] = 1;

  // --- Aggiorno SOLO se cambia ---
  for (int i = 0; i < 6; i++) {
    if (newState[i] != buttonState[i]) {
      GameController.setButton(i, newState[i]);
      buttonState[i] = newState[i];
    }
  }
  delay (20);
}