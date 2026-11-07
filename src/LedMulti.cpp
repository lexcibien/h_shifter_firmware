#include <LedMulti.h>
#include <array>
#include <pico/time.h>
#include <utility>

uint32_t nowMs() { return to_ms_since_boot(get_absolute_time()); }

void LedMulti::begin() {
  led.setPin(PICO_DEFAULT_WS2812_PIN);
  led.updateLength(1);
  // NOLINTNEXTLINE(bugprone-signed-bitwise)
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
  step = 1;
  lastChange = nowMs();
  led.setPixelColor(0, std::to_underlying(Color::SEQUENTIAL));
  led.show();
}

void LedMulti::update() {
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
