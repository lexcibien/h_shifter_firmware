#pragma once

#include <Adafruit_NeoPixel.h>
#include <pico/stdlib.h>
#include <utility>

enum class Color : uint32_t {
  H_SHIFTER = (255 << 16) | (128 << 8) | 0,  // ORANGE
  SEQUENTIAL = (255 << 16) | (0 << 8) | 255, // PURPLE
  DANGER = (255 << 16) | (0 << 8) | 0,       // RED
  WARN = (255 << 16) | (215 << 8) | 0,       // YELLOW
  OK = (0 << 16) | (255 << 8) | 0,           // GREEN
};

class LedMulti {
private:
  Adafruit_NeoPixel led;
  uint32_t lastChange = 0;
  uint8_t step = 0;
  bool running = false;

public:
  void begin() {
    led.setPin(PIN_NEOPIXEL);
    led.updateLength(1);
    led.updateType(NEO_GRB + NEO_KHZ800);
    led.begin();
    led.clear();
    led.show();
  }

  void showLed(Color color) {
    led.setPixelColor(0, std::to_underlying(color));
    led.show();
  }

  void connectHandle() {
    running = true;
    step = 0;
    lastChange = millis();
    led.setPixelColor(0, std::to_underlying(Color::SEQUENTIAL));
    led.show();
  }

  void update() {
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
};
