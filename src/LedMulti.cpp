#include <LedMulti.h>
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
    lastChange = millis();

    switch (step) {
      using enum Color;
      case 0: led.setPixelColor(0, std::to_underlying(SEQUENTIAL)); break;
      case 1: led.setPixelColor(0, std::to_underlying(OK)); break;
      case 2: led.setPixelColor(0, std::to_underlying(WARN)); break;
      default:
        led.clear();
        led.show();
        running = false;
        return;
    }

    led.show();
    step++;
  }
}
