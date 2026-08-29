#include <Arduino.h>
#include <HidReport.h>

void HidReport::begin() {
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

  bool HidReport::mounted() { return TinyUSBDevice.mounted(); }
  bool HidReport::ready() { return usbHid.ready(); }

  void HidReport::send(const ShifterModel::ButtonState& buttonState) {
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
