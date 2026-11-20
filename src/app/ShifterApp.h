#pragma once

#include "led/StatusLed.h"
#include "shifter/ShifterModel.h"
#include "usb/HidReport.h"

class ShifterApp {
public:
  void initialize();
  [[noreturn]] void run();

private:
  void update();

  HidReport hidReport;
  StatusLed statusLed;

  ShifterModel::ShifterConfig shifterConfig{};
  ShifterModel::ButtonState previousButtonState{};

  bool previousReportValid = false;
  bool wasMounted = false;
  bool hasMounted = false;
  bool previousSequentialMode = false;
};
