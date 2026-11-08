#include "ShifterApp.h"
#include "diagnostics/Log.h"
#include "shifter/ShifterInput.h"
#include "shifter/ShifterLogic.h"
#if defined(H_SHIFTER_DEBUG) && H_SHIFTER_LOG
#include "diagnostics/Debug.h"
#endif
#ifdef TINYUSB_NEED_POLLING_TASK
#include <arduino/Adafruit_USBD_Device.h>
#endif

static constexpr Color modeColor(bool sequentialEnabled) { return sequentialEnabled ? Color::SEQUENTIAL : Color::H_SHIFTER; }

void ShifterApp::initialize() {
#if H_SHIFTER_LOG
  Serial.begin(115200);
#else
  Serial.end();
#endif
  hidReport.begin();
  statusLed.begin();
  statusLed.showLed(Color::WARN);
  ShifterInput::begin();

  shifterConfig = ShifterInput::configuration();
  previousSequentialMode = shifterConfig.sequentialEnabled;

  if (shifterConfig.handleConnected) {
    statusLed.connectHandle();
  } else {
    statusLed.showLed(modeColor(shifterConfig.sequentialEnabled));
  }

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  Log::info("system: project=V1");
#else
  Log::info("system: project=V2");
#endif

  Log::info("system: initialized");
  Log::info(shifterConfig.handleConnected ? "handle: connected" : "handle: not-connected");
  Log::info(ShifterModel::ENABLE_REVERSE ? "reverse: enabled" : "reverse: disabled");
  Log::info(shifterConfig.sequentialEnabled ? "shifter: mode=sequential" : "shifter: mode=h-pattern");
}

void ShifterApp::update() {
#ifdef TINYUSB_NEED_POLLING_TASK
  TinyUSBDevice.task();
#endif

  const bool wasLedSignaling = statusLed.isSignaling();
  statusLed.update();
  if (wasLedSignaling && !statusLed.isSignaling()) {
    statusLed.showLed(modeColor(shifterConfig.sequentialEnabled));
  }

  const bool mounted = HidReport::mounted();
  if (!mounted) {
    wasMounted = false;
    previousReportValid = false;
    delay(10);
    return;
  }

  if (!wasMounted) {
    Log::event(hasMounted ? "usb: remounted" : "usb: mounted");
    wasMounted = true;
    hasMounted = true;
  }

  const ShifterModel::InputState inputs = ShifterInput::readInputs();
  shifterConfig = ShifterInput::configuration();

#if defined(H_SHIFTER_DEBUG) && H_SHIFTER_LOG
#ifdef DEBUG_ANALOG
  ShifterInput::AnalogState inputAnalog = ShifterInput::readAnalogInput(); //* The button will not work digitally
  Debug::printRawAnalog(inputAnalog);
#endif
  Debug::printRawInputs(inputs);
#endif

  if (shifterConfig.sequentialEnabled != previousSequentialMode) {
    Log::event(shifterConfig.sequentialEnabled ? "shifter: mode=sequential" : "shifter: mode=h-pattern");
    previousSequentialMode = shifterConfig.sequentialEnabled;
    if (!statusLed.isSignaling()) {
      statusLed.showLed(modeColor(shifterConfig.sequentialEnabled));
    }
  }

  const ShifterModel::ButtonState buttonState = ShifterLogic::resolveButtonState(inputs, shifterConfig);

  if (!hidReport.ready()) {
    delay(10);
    return;
  }

  if (!previousReportValid || buttonState != previousButtonState) {
    if (hidReport.send(buttonState)) {

#if defined(H_SHIFTER_DEBUG) && H_SHIFTER_LOG
      Debug::printOutputState(buttonState);
#endif

      previousButtonState = buttonState;
      previousReportValid = true;
    }
  }

  delay(20);
}
