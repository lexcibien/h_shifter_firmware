#include <LedMulti.h>
#include <array>
#include <utility>

void LedMulti::begin() {
  led.setPin(PIN_NEOPIXEL);
  led.updateLength(1);
  led.updateType(NEO_GRB + NEO_KHZ800);
  led.begin();
  led.clear();
  led.show();
}

void LedMulti::showLed(Color color) {
  led.setPixelColor(0, std::to_underlying(color));
  led.show();
}

void LedMulti::connectHandle() {
  running = true;
  step = 0;
  lastChange = millis();
  led.setPixelColor(0, std::to_underlying(Color::SEQUENTIAL));
  led.show();
}

void LedMulti::update() {
  if (!running) {
    return;
  }

  if (millis() - lastChange >= 500) { // 500 ms por cor
    using enum Color;
    lastChange = millis();

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
