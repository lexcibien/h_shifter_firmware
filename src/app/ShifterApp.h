#pragma once

#include "config/DeviceConfig.h"
#include "led/StatusLed.h"
#include "shifter/ShifterModel.h"
#include "storage/ConfigStore.h"
#include "usb/HidReport.h"
#include "usb/WebUsbProtocol.h"

class ShifterApp : private WebUsbProtocol::Handler {
public:
  ShifterApp();
  void initialize();
  [[noreturn]] void run();

private:
  void update();
  void updateConfigurationUnlock();
  void applyConfig(const DeviceConfig& config);
  void showModeColor();
  WebUsbProtocol::Status
  handleWebUsbCommand(WebUsbProtocol::Command command, std::span<const uint8_t> request, std::span<uint8_t> response, uint8_t& responseSize) override;

  HidReport hidReport;
  StatusLed statusLed;
  ConfigStore configStore;
  WebUsbProtocol webUsbProtocol;
  DeviceConfig deviceConfig = DeviceConfig::defaults();

  ShifterModel::ShifterConfig shifterConfig{};
  ShifterModel::InputState currentInputs{};
  ShifterModel::ButtonState currentButtonState{};
  ShifterModel::ButtonState previousButtonState{};

  bool previousReportValid = false;
  bool wasMounted = false;
  bool hasMounted = false;
  bool previousSequentialMode = false;
  bool configurationUnlocked = false;
  bool unlockGestureActive = false;
  uint32_t unlockGestureStartedAt = 0;
  uint32_t unlockLedUntil = 0;
  uint32_t rebootAt = 0;
};
