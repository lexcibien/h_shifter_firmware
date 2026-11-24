#include "StatusLed.h"
#include <array>
#include <pico/time.h>
#include <utility>

uint32_t nowMs() { return to_ms_since_boot(get_absolute_time()); }

void StatusLed::begin() {
  if (!led.begin()) {
    return;
  }
  led.clear();
}

void StatusLed::showLed(Color color) {
  const uint32_t rgb = std::to_underlying(color);
  led.show(static_cast<uint8_t>(rgb >> 16U), static_cast<uint8_t>(rgb >> 8U), static_cast<uint8_t>(rgb));
}

void StatusLed::connectHandle() {
  running = true;
  step = 1;
  lastChange = nowMs();
  showLed(Color::SEQUENTIAL);
}

void StatusLed::update() {
  if (!running) {
    return;
  }

  if (nowMs() - lastChange >= 500) { // 500 ms por cor
    using enum Color;
    lastChange = nowMs();

    static constexpr std::array connectionColors = { SEQUENTIAL, OK, WARN };
    if (step >= connectionColors.size()) {
      led.clear();
      running = false;
      return;
    }

    showLed(connectionColors.at(step));
    step++;
  }
}
