#include "hid/Adafruit_USBD_HID.h"
#include <Arduino.h>
#include <array>

class ShifterModel {
public:
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

  static constexpr uint8_t BUTTON_COUNT = COUNT;
  static constexpr bool ENABLE_REVERSE = true;
  using ButtonState = std::array<bool, BUTTON_COUNT>;

  struct InputState {
    bool swFront = false;
    bool swLeft = false;
    bool swRight = false;
    bool swBack = false;
    bool swReverse = false;
    bool swRange = false;
    bool swSplit = false;
    bool btnEngineBrake = false;
  };

  struct ShifterConfig {
    bool sequentialEnabled = false;
    bool handleConnected = false;
  };
};

class HidReport {
private:
  static constexpr uint8_t HID_BUTTON_COUNT = ShifterModel::BUTTON_COUNT;
  struct __attribute__((packed)) ButtonsReport {
    std::array<uint8_t, 2> buttons;
  };

  Adafruit_USBD_HID usbHid;
  ButtonsReport buttonReport = {};
  static constexpr std::array<uint8_t, 30> hidReportDescriptor = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x04, // Usage (Joystick)
    0xA1, 0x01, // Collection (Application)

    0x05, 0x09,             // Usage Page (Button)
    0x19, 0x01,             // Usage Minimum (Button 1)
    0x29, HID_BUTTON_COUNT, // Usage Maximum (Button HID_BUTTON_COUNT)

    0x15, 0x00, // Logical Minimum (0)
    0x25, 0x01, // Logical Maximum (1)

    0x75, 0x01,             // Report Size (1)
    0x95, HID_BUTTON_COUNT, // Report Count (HID_BUTTON_COUNT)
    0x81, 0x02,             // Input (Data, Variable, Absolute)

    // Padding: 4 bits
    0x75, 0x04, // Report Size (4)
    0x95, 0x01, // Report Count (1)
    0x81, 0x01, // Input (Constant)

    0xC0 // End Collection
  };

public:
  void begin() {
    if (!TinyUSBDevice.isInitialized()) {
      TinyUSBDevice.begin(0);
    }

    usbHid.setPollInterval(2);
    usbHid.setReportDescriptor(hidReportDescriptor.data(), hidReportDescriptor.size());
    usbHid.begin();

    if (TinyUSBDevice.mounted()) {
      TinyUSBDevice.detach();
      delay(10);
      TinyUSBDevice.attach();
    }
  }

  static bool mounted() { return TinyUSBDevice.mounted(); }
  bool ready() { return usbHid.ready(); }

  void send(const ShifterModel::ButtonState& buttonState) {
    uint16_t buttonsMask = 0;

    for (uint8_t i = 0; i < ShifterModel::BUTTON_COUNT; ++i) {
      if (buttonState.at(i)) {
        buttonsMask |= (1U << i);
      }
    }

    buttonReport.buttons.at(0) = static_cast<uint8_t>(buttonsMask & 0xFFU);
    buttonReport.buttons.at(1) = static_cast<uint8_t>((buttonsMask >> 8U) & 0x0FU);
    usbHid.sendReport(0, &buttonReport, sizeof(buttonReport));
  }
};

class ShifterInput {
private:
  static bool detectHandleConnection() {
    int adc = analogRead(SW_KNOB_RANGE);
    return adc > 300 && adc < 600;
  }

  bool handleConnected = false;

public:
  void begin() {
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

  [[nodiscard]] ShifterModel::InputState readInputs() const {
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

  [[nodiscard]] ShifterModel::ShifterConfig configuration() const {
    return { .sequentialEnabled = digitalRead(SW_ENABLE_SEQUENTIAL) == LOW, .handleConnected = handleConnected };
  }
};

class ShifterLogic {
private:
  using InputState = ShifterModel::InputState;
  using Config = ShifterModel::ShifterConfig;
  using ButtonState = ShifterModel::ButtonState;
  using Buttons = ShifterModel::ControllerButtons;

  void resolveLateralGears(const InputState& inputs, ButtonState& buttonState, bool& combFrontUsed, bool& combBackUsed) const {
    if (inputs.swBack && inputs.swRight && !isReverseGear) {
      buttonState[Buttons::GEAR_1] = true;
      combFrontUsed = true;
    }
    if (inputs.swFront && inputs.swRight) {
      buttonState[Buttons::GEAR_2] = true;
      combBackUsed = true;
    }
    if (inputs.swBack && inputs.swLeft) {
      buttonState[Buttons::GEAR_5] = true;
      combFrontUsed = true;
    }
    if (inputs.swFront && inputs.swLeft) {
      buttonState[Buttons::GEAR_6] = true;
      combBackUsed = true;
    }
  }

  void activateReverseGear(const InputState& inputs, ButtonState& buttonState) {
    if (ShifterModel::ENABLE_REVERSE && inputs.swReverse && buttonState[Buttons::GEAR_1]) {
      buttonState[Buttons::GEAR_1] = false;
      isReverseGear = true;
    }
  }

  void applyReverseGear(ButtonState& buttonState, bool& combFrontUsed) const {
    if (isReverseGear) {
      buttonState[Buttons::GEAR_R] = true;
      combFrontUsed = true;
    }
  }

  static void
  resolveSequentialGears(const InputState& inputs, const Config& config, ButtonState& buttonState, bool combFrontUsed, bool combBackUsed) {
    if (config.sequentialEnabled) {
      if (inputs.swFront && !combFrontUsed) {
        buttonState[Buttons::SW_SEQ_MINUS] = true;
      }
      if (inputs.swBack && !combBackUsed) {
        buttonState[Buttons::SW_SEQ_PLUS] = true;
      }
    }
  }

  static void resolveCenterGears(const InputState& inputs, ButtonState& buttonState, bool combFrontUsed, bool combBackUsed) {
    if (inputs.swBack && !combFrontUsed) {
      buttonState[Buttons::GEAR_3] = true;
    }
    if (inputs.swFront && !combBackUsed) {
      buttonState[Buttons::GEAR_4] = true;
    }
  }

  void resetReverseGear(const InputState& inputs) {
    if (!inputs.swFront && !inputs.swLeft && !inputs.swRight && !inputs.swBack) {
      isReverseGear = false;
    }
  }

  static void applyHandleButtons(const InputState& inputs, const Config& config, ButtonState& buttonState) {
    if (config.handleConnected) {
      buttonState[Buttons::RANGE] = inputs.swRange;
      buttonState[Buttons::SPLIT] = inputs.swSplit;
      buttonState[Buttons::ENGINE_BRAKE] = inputs.btnEngineBrake;
    }
  }

  bool isReverseGear = false;

public:
  ButtonState resolveButtonState(const InputState& inputs, const Config& config) {
    ButtonState buttonState = {};
    bool combFrontUsed = false;
    bool combBackUsed = false;

    resolveLateralGears(inputs, buttonState, combFrontUsed, combBackUsed);
    activateReverseGear(inputs, buttonState);
    applyReverseGear(buttonState, combFrontUsed);
    resolveSequentialGears(inputs, config, buttonState, combFrontUsed, combBackUsed);
    resolveCenterGears(inputs, buttonState, combFrontUsed, combBackUsed);
    resetReverseGear(inputs);
    applyHandleButtons(inputs, config, buttonState);

    return buttonState;
  }
};

class Debug {
public:
  static void printRawInputs(const ShifterModel::InputState& inputs) {
    Serial.print("RAW:");
    Serial.print(inputs.swFront);
    Serial.print(", ");
    Serial.print(inputs.swLeft);
    Serial.print(", ");
    Serial.print(inputs.swRight);
    Serial.print(", ");
    Serial.print(inputs.swBack);
    Serial.print(", ");
    Serial.print(inputs.swReverse);
    Serial.print(", ");
    Serial.print(inputs.swRange);
    Serial.print(", ");
    Serial.print(inputs.swSplit);
    Serial.print(", ");
    Serial.print(inputs.btnEngineBrake);
    Serial.println();
  }

  static void printOutputState(const ShifterModel::ButtonState& buttonState) {
    Serial.print("OUT: ");
    for (uint8_t i = 0; i < ShifterModel::BUTTON_COUNT; ++i) {
      Serial.print(buttonState.at(i));
    }
    Serial.println();
  }
};

// NOLINTBEGIN (cppcoreguidelines-avoid-non-const-global-variables)
HidReport hidReport;
ShifterInput shifterInput;
ShifterLogic shifterLogic;
Debug debug;
ShifterModel::ButtonState previousButtonState = {};
ShifterModel::ShifterConfig shifterConfig;
// NOLINTEND

void setup() {
  Serial.begin(115200);
  hidReport.begin();
  shifterInput.begin();
  shifterConfig = shifterInput.configuration();

  if (!shifterConfig.handleConnected) {
    Serial.println("INFO: Truck shifter handle not connected");
  } else {
    Serial.println("OK: Handle detected");
  }

  Serial.println(ShifterModel::ENABLE_REVERSE ? "OK: Rear gear is enabled" : "INFO: Rear gear is disabled");
  Serial.println(shifterConfig.sequentialEnabled ? "OK: The current gear output is sequential" : "INFO: The current gear output is H-Shifter");
}

void loop() {
#ifdef TINYUSB_NEED_POLLING_TASK
  TinyUSBDevice.task();
#endif

  if (!HidReport::mounted()) {
    return;
  }

  ShifterModel::InputState inputs = shifterInput.readInputs();

#ifdef DEBUG
  Debug::printRawInputs(inputs);
#endif

  ShifterModel::ButtonState buttonState = shifterLogic.resolveButtonState(inputs, shifterConfig);

  if (!hidReport.ready()) {
    return;
  }

  if (buttonState != previousButtonState) {
    hidReport.send(buttonState);
#ifdef DEBUG
    Debug::printOutputState(buttonState);
#endif
    previousButtonState = buttonState;
  }

  delay(20);
}
