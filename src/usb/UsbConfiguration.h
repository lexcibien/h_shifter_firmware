#pragma once

#include "usb/ConfigProtocol.h"

class UsbConfiguration {
public:
  explicit UsbConfiguration(ConfigProtocol::Handler& handler);
  void update();
  void reset();

private:
  ConfigProtocol protocol_;
};
