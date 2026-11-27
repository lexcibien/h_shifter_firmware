#pragma once

#include "usb/ConfigProtocol.h"

class UsbConfiguration {
public:
  explicit UsbConfiguration(ConfigProtocol::Handler& handler);
  void update();
  void reset();

private:
  ConfigProtocol web_;
#if H_SHIFTER_CONFIG_CDC
  ConfigProtocol serial_;
#endif
};
