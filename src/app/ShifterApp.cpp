#include "ShifterApp.h"
#include "diagnostics/Log.h"
#include "shifter/ShifterInput.h"
#include "shifter/ShifterLogic.h"
#if defined(DEBUG) && H_SHIFTER_LOG
#include "diagnostics/Debug.h"
#endif
#ifdef TINYUSB_NEED_POLLING_TASK
#include <device/usbd.h>
#endif
#if H_SHIFTER_LOG
#include <pico/stdio.h>
#endif
#include <pico/time.h>

constexpr Color modeColor(bool sequentialEnabled) { return sequentialEnabled ? Color::SEQUENTIAL : Color::H_SHIFTER; }

void ShifterApp::initialize() {
#if H_SHIFTER_LOG
  stdio_init_all();
#endif
  HidReport::begin();
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

[[noreturn]] void ShifterApp::run() {
  while (true) {
    update();
  }
}

void ShifterApp::update() {
#ifdef TINYUSB_NEED_POLLING_TASK
  tud_task();
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
    sleep_ms(10);
    return;
  }

  if (!wasMounted) {
    Log::event(hasMounted ? "usb: remounted" : "usb: mounted");
    wasMounted = true;
    hasMounted = true;
  }

  const ShifterModel::InputState inputs = ShifterInput::readInputs();
  shifterConfig = ShifterInput::configuration();

#if defined(DEBUG) && H_SHIFTER_LOG
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

  if (!HidReport::ready()) {
    sleep_ms(10);
    return;
  }

  if (!previousReportValid || buttonState != previousButtonState) {
    if (hidReport.send(buttonState)) {

#if defined(DEBUG) && H_SHIFTER_LOG
      Debug::printOutputState(buttonState);
#endif

      previousButtonState = buttonState;
      previousReportValid = true;
    }
  }

  sleep_ms(20);
}
