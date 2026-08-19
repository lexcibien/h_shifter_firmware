#include "hid/Adafruit_USBD_HID.h"
#include <Arduino.h>
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

constexpr uint8_t BUTTON_COUNT = COUNT;
constexpr uint8_t HID_BUTTON_COUNT = BUTTON_COUNT;

struct __attribute__((packed)) ButtonsReport {
  uint8_t buttons[2];
};

uint8_t const desc_hid_report[] = {
  0x05, 0x01,             // Usage Page (Generic Desktop)
  0x09, 0x05,             // Usage (Gamepad)
  0xA1, 0x01,             // Collection (Application)
  0x05, 0x09,             // Usage Page (Button)
  0x19, 0x01,             // Usage Minimum (Button 1)
  0x29, HID_BUTTON_COUNT, // Usage Maximum (Button N)
  0x15, 0x00,             // Logical Minimum (0)
  0x25, 0x01,             // Logical Maximum (1)
  0x75, 0x01,             // Report Size (1)
  0x95, HID_BUTTON_COUNT, // Report Count (N)
  0x81, 0x02,             // Input (Data, Var, Abs)
  0xC0,                   // End Collection
};

Adafruit_USBD_HID usb_hid;
ButtonsReport buttonReport = {};

bool detectHandleConnection();

bool handleConnected = false;
bool isReverseGear = false;
std::array<bool, BUTTON_COUNT> prevButtonState = {};

bool swEnableReverse;
bool swEnableSequential;

void sendButtonReport(const std::array<bool, BUTTON_COUNT>& buttonState) {
  uint16_t buttonsMask = 0;

  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    if (buttonState.at(i)) {
      buttonsMask |= (1U << i);
    }
  }

  buttonReport.buttons[0] = static_cast<uint8_t>(buttonsMask & 0xFFU);
  buttonReport.buttons[1] = static_cast<uint8_t>((buttonsMask >> 8U) & 0x0FU);

  usb_hid.sendReport(0, &buttonReport, sizeof(buttonReport));
}

void setup() {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  Serial.begin(115200);

  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }

  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.begin();

  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

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
#ifdef TINYUSB_NEED_POLLING_TASK
  TinyUSBDevice.task();
#endif

  if (!TinyUSBDevice.mounted()) {
    return;
  }

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

  if (!usb_hid.ready()) {
    return;
  }

#ifdef DEBUG
  Serial.print("OUT: ");
#endif
  if (newButtonState != prevButtonState) {
    sendButtonReport(newButtonState);
#ifdef DEBUG
    for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
      Serial.print(newButtonState.at(i));
    }
#endif
    prevButtonState = newButtonState;
  }
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
