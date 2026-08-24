#include "bsp/board_api.h"
#include "common/tusb_types.h"
#include "device/usbd.h"
#include "tusb.h"
#include <HidReport.h>

void HidReport::begin() {
  if (!tud_inited()) {
    const tusb_rhport_init_t rh_init = { .role = TUSB_ROLE_DEVICE, .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL };
    board_init();
    tusb_init(0, &rh_init);
  }

  if (tud_mounted()) {
    tud_disconnect();
    sleep_ms(10);
    tud_connect();
  }
}

bool HidReport::mounted() { return tud_mounted(); }
bool HidReport::ready() { return tud_hid_ready(); }

void HidReport::send(const ShifterModel::ButtonState& buttonState) {
  uint16_t buttonsMask = 0;

  for (uint8_t i = 0; i < ShifterModel::BUTTON_COUNT; ++i) {
    if (buttonState.at(i)) {
      buttonsMask |= (1U << i);
    }
  }

  buttonReport.buttons.at(0) = static_cast<uint8_t>(buttonsMask & 0xFFU);
  buttonReport.buttons.at(1) = static_cast<uint8_t>((buttonsMask >> 8U) & 0x0FU);
  tud_hid_report(0, &buttonReport, sizeof(buttonReport));
}
