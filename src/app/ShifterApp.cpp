#include "ShifterApp.h"
#include "diagnostics/Log.h"
#include "shifter/ShifterInput.h"
#include "shifter/ShifterLogic.h"
#if defined(H_SHIFTER_DEBUG) && H_SHIFTER_LOG
#include "diagnostics/Debug.h"
#endif
#ifdef TINYUSB_NEED_POLLING_TASK
#include <device/usbd.h>
#endif
#if H_SHIFTER_LOG
#include <pico/stdio.h>
#endif
#include <algorithm>
#include <hardware/watchdog.h>
#include <pico/time.h>

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
constexpr uint16_t CAP_DEFAULT_MODE = 1U << 0U;
constexpr uint16_t CAP_SEQUENTIAL_HOLD = 1U << 1U;
#endif
constexpr uint16_t CAP_REVERSE = 1U << 2U;
constexpr uint16_t CAP_HANDLE = 1U << 3U;
constexpr uint16_t CAP_COLORS = 1U << 4U;
constexpr uint16_t CAP_DEBOUNCE = 1U << 5U;

static void writeU16(std::span<uint8_t> bytes, size_t offset, uint16_t value) {
  bytes[offset] = static_cast<uint8_t>(value);
  bytes[offset + 1] = static_cast<uint8_t>(value >> 8U);
}

static void writeU32(std::span<uint8_t> bytes, size_t offset, uint32_t value) {
  for (size_t index = 0; index < sizeof(value); ++index) {
    bytes[offset + index] = static_cast<uint8_t>(value >> (index * 8U));
  }
}

uint16_t rawInputMask(const ShifterModel::InputState& inputs) {
  uint16_t mask = 0;
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  mask |= static_cast<uint16_t>(inputs.swFront << 0U);
  mask |= static_cast<uint16_t>(inputs.swBack << 1U);
  mask |= static_cast<uint16_t>(inputs.swLeft << 2U);
  mask |= static_cast<uint16_t>(inputs.swRight << 3U);
  mask |= static_cast<uint16_t>(inputs.swReverse << 4U);
#else
  mask |= static_cast<uint16_t>(inputs.hPatternMode << 0U);
  mask |= static_cast<uint16_t>(inputs.sw3 << 1U);
  mask |= static_cast<uint16_t>(inputs.sw4 << 2U);
  mask |= static_cast<uint16_t>(inputs.sw5 << 3U);
  mask |= static_cast<uint16_t>(inputs.sw6 << 4U);
#endif
  mask |= static_cast<uint16_t>(inputs.swRange << 5U);
  mask |= static_cast<uint16_t>(inputs.swSplit << 6U);
  mask |= static_cast<uint16_t>(inputs.btnEngineBrake << 7U);
  return mask;
}

uint16_t buttonMask(const ShifterModel::ButtonState& buttons) {
  uint16_t mask = 0;
  for (size_t index = 0; index < buttons.size(); ++index) {
    mask |= static_cast<uint16_t>(buttons.at(index) << index);
  }
  return mask;
}

ShifterApp::ShifterApp() : webUsbProtocol(this) {}

void ShifterApp::showModeColor() { statusLed.showRgb(shifterConfig.sequentialEnabled ? deviceConfig.sequentialModeColor : deviceConfig.hModeColor); }

void ShifterApp::initialize() {
#if H_SHIFTER_LOG
  stdio_init_all();
#endif
  const ConfigStore::LoadResult stored = configStore.load();
  deviceConfig = stored.config;

  HidReport::begin();
  statusLed.begin();
  statusLed.showLed(Color::WARN);
  ShifterInput::begin(deviceConfig);

  shifterConfig = ShifterInput::configuration();
  previousSequentialMode = shifterConfig.sequentialEnabled;
  if (shifterConfig.handleConnected) {
    statusLed.connectHandle();
  } else {
    showModeColor();
  }

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  Log::info("system: project=V1");
#else
  Log::info("system: project=V2");
#endif
  Log::info(stored.source == ConfigStore::Source::FLASH ? "config: loaded-from-flash" : "config: using-defaults");
  Log::info("system: initialized");
  Log::info(shifterConfig.handleConnected ? "handle: connected" : "handle: not-connected");
  Log::info(deviceConfig.reverseEnabled ? "reverse: enabled" : "reverse: disabled");
  Log::info(shifterConfig.sequentialEnabled ? "shifter: mode=sequential" : "shifter: mode=h-pattern");
}

[[noreturn]] void ShifterApp::run() {
  while (true) {
    update();
  }
}

void ShifterApp::updateConfigurationUnlock() {
  const uint32_t now = to_ms_since_boot(get_absolute_time());
  if (!ShifterInput::configurationUnlockGesture()) {
    unlockGestureActive = false;
    unlockGestureStartedAt = 0;
    return;
  }
  if (!unlockGestureActive) {
    unlockGestureActive = true;
    unlockGestureStartedAt = now;
  } else if (!configurationUnlocked && now - unlockGestureStartedAt >= 1000) {
    configurationUnlocked = true;
    unlockLedUntil = now + 500;
    statusLed.showLed(Color::OK);
    Log::event("webusb: configuration-unlocked");
  }
}

void ShifterApp::update() {
#ifdef TINYUSB_NEED_POLLING_TASK
  tud_task();
#endif
  webUsbProtocol.update();

  const uint32_t now = to_ms_since_boot(get_absolute_time());
  if (rebootAt != 0 && now >= rebootAt) {
    watchdog_reboot(0, 0, 0);
  }

  const bool wasLedSignaling = statusLed.isSignaling();
  statusLed.update();
  if ((wasLedSignaling && !statusLed.isSignaling()) || (unlockLedUntil != 0 && now >= unlockLedUntil)) {
    unlockLedUntil = 0;
    showModeColor();
  }

  const bool mounted = HidReport::mounted();
  if (!mounted) {
    if (wasMounted) {
      configurationUnlocked = false;
      webUsbProtocol.reset();
    }
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

  currentInputs = ShifterInput::readInputs();
  shifterConfig = ShifterInput::configuration();
  updateConfigurationUnlock();

#if defined(H_SHIFTER_DEBUG) && H_SHIFTER_LOG
  Debug::printRawInputs(currentInputs);
#endif

  if (shifterConfig.sequentialEnabled != previousSequentialMode) {
    Log::event(shifterConfig.sequentialEnabled ? "shifter: mode=sequential" : "shifter: mode=h-pattern");
    previousSequentialMode = shifterConfig.sequentialEnabled;
    if (!statusLed.isSignaling() && unlockLedUntil == 0) {
      showModeColor();
    }
  }

  currentButtonState = ShifterLogic::resolveButtonState(currentInputs, shifterConfig);
  if (!HidReport::ready()) {
    sleep_ms(10);
    return;
  }
  if (!previousReportValid || currentButtonState != previousButtonState) {
    if (hidReport.send(currentButtonState)) {
#if defined(H_SHIFTER_DEBUG) && H_SHIFTER_LOG
      Debug::printOutputState(currentButtonState);
#endif
      previousButtonState = currentButtonState;
      previousReportValid = true;
    }
  }
  sleep_ms(20);
}

void ShifterApp::applyConfig(const DeviceConfig& config) {
  deviceConfig = config;
  ShifterInput::setConfig(config);
  shifterConfig = ShifterInput::configuration();
  if (unlockLedUntil == 0 && !statusLed.isSignaling()) {
    showModeColor();
  }
}

WebUsbProtocol::Status ShifterApp::handleWebUsbCommand(WebUsbProtocol::Command command, std::span<const uint8_t> request, std::span<uint8_t> response,
                                                       uint8_t& responseSize) {
  using Command = WebUsbProtocol::Command;
  using Status = WebUsbProtocol::Status;
  responseSize = 0;

  const bool mutableCommand =
      command == Command::SAVE_CONFIG || command == Command::RESTORE_DEFAULTS || command == Command::REDETECT_HANDLE || command == Command::REBOOT;
  if (mutableCommand && !configurationUnlocked) {
    return Status::LOCKED;
  }

  switch (command) {
    case Command::GET_INFO: {
      if (!request.empty()) {
        return Status::BAD_FRAME;
      }
      uint16_t capabilities = static_cast<uint16_t>(static_cast<uint16_t>(CAP_REVERSE | CAP_HANDLE) | CAP_COLORS) | CAP_DEBOUNCE;
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
      capabilities |= CAP_DEFAULT_MODE | CAP_SEQUENTIAL_HOLD;
#endif
      response[0] = DeviceConfig::SCHEMA_VERSION;
      response[1] = SHIFTER_PROJECT_VERSION;
      response[2] = DeviceConfig::MAX_FORWARD_GEARS;
      writeU16(response, 3, capabilities);
      response[5] = 1;
      response[6] = 0;
      response[7] = 0;
      responseSize = 8;
      return Status::OK;
    }
    case Command::GET_CONFIG: {
      if (!request.empty()) {
        return Status::BAD_FRAME;
      }
      const auto encoded = deviceConfig.encode();
      std::ranges::copy(encoded, response.begin());
      writeU32(response, encoded.size(), configStore.generation());
      response[encoded.size() + 4] = static_cast<uint8_t>(configStore.source());
      responseSize = encoded.size() + 5;
      return Status::OK;
    }
    case Command::SAVE_CONFIG: {
      DeviceConfig requested;
      if (!DeviceConfig::decode(request, requested)) {
        return Status::INVALID_CONFIG;
      }
      if (!configStore.save(requested)) {
        return Status::FLASH_ERROR;
      }
      applyConfig(requested);
      writeU32(response, 0, configStore.generation());
      responseSize = 4;
      Log::event("config: saved");
      return Status::OK;
    }
    case Command::RESTORE_DEFAULTS: {
      if (!request.empty()) {
        return Status::BAD_FRAME;
      }
      const DeviceConfig defaults = DeviceConfig::defaults();
      if (!configStore.save(defaults)) {
        return Status::FLASH_ERROR;
      }
      applyConfig(defaults);
      writeU32(response, 0, configStore.generation());
      responseSize = 4;
      Log::event("config: factory-defaults-restored");
      return Status::OK;
    }
    case Command::GET_STATUS: {
      if (!request.empty()) {
        return Status::BAD_FRAME;
      }
      uint8_t flags = 0;
      flags |= static_cast<uint8_t>(configurationUnlocked << 0U);
      flags |= static_cast<uint8_t>(shifterConfig.sequentialEnabled << 1U);
      flags |= static_cast<uint8_t>(shifterConfig.handleConnected << 2U);
      response[0] = flags;
      writeU16(response, 1, static_cast<uint16_t>(ShifterInput::readAnalogInput().adc));
      writeU16(response, 3, rawInputMask(currentInputs));
      writeU16(response, 5, buttonMask(currentButtonState));
      responseSize = 7;
      return Status::OK;
    }
    case Command::REDETECT_HANDLE: {
      if (!request.empty()) {
        return Status::BAD_FRAME;
      }
      const bool connected = ShifterInput::redetectHandle();
      shifterConfig = ShifterInput::configuration();
      response[0] = static_cast<uint8_t>(connected);
      writeU16(response, 1, static_cast<uint16_t>(ShifterInput::readAnalogInput().adc));
      responseSize = 3;
      Log::event(connected ? "handle: connected" : "handle: not-connected");
      return Status::OK;
    }
    case Command::REBOOT:
      if (!request.empty()) {
        return Status::BAD_FRAME;
      }
      rebootAt = to_ms_since_boot(get_absolute_time()) + 250;
      return Status::OK;
    default: return Status::UNSUPPORTED;
  }
}
