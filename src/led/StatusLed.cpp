#include "StatusLed.h"
#include <array>
#include <utility>

static uint32_t nowMs() { return millis(); }

void StatusLed::begin() {
  led.setPin(PIN_NEOPIXEL);
  led.updateLength(1);
  // NOLINTNEXTLINE(bugprone-signed-bitwise)
  led.updateType(NEO_GRB + NEO_KHZ800);
  led.begin();
  led.clear();
  led.show();
}

void StatusLed::showLed(Color color) {
  led.setPixelColor(0, std::to_underlying(color));
  led.show();
}

void StatusLed::connectHandle() {
  running = true;
  step = 1;
  lastChange = nowMs();
  led.setPixelColor(0, std::to_underlying(Color::SEQUENTIAL));
  led.show();
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
      led.show();
      running = false;
      return;
    }

    led.setPixelColor(0, std::to_underlying(connectionColors.at(step)));
    led.show();
    step++;
  }
}
