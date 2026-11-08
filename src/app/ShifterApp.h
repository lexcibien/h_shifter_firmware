#pragma once

#include "led/StatusLed.h"
#include "shifter/ShifterModel.h"
#include "usb/HidReport.h"

class ShifterApp {
public:
  void initialize();
  void update();

private:
  HidReport hidReport;
  StatusLed statusLed;

  ShifterModel::ShifterConfig shifterConfig{};
  ShifterModel::ButtonState previousButtonState{};

  bool previousReportValid = false;
  bool wasMounted = false;
  bool hasMounted = false;
  bool previousSequentialMode = false;
};
