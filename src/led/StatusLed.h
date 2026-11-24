#pragma once

#include "Ws2812Led.h"
#include <cstdint>
#include <pico/stdlib.h>

enum class Color : uint32_t {
  // (red << 16) | (green << 8) | blue
  H_SHIFTER = (255U << 16U) | (128U << 8U), // ORANGE
  SEQUENTIAL = (255U << 16U) | 255U,        // PURPLE
  DANGER = 255U << 16U,                     // RED
  WARN = (255U << 16U) | (215U << 8U),      // YELLOW
  OK = 255U << 8U,                          // GREEN
};

class StatusLed {
private:
  Ws2812Led led{ PICO_DEFAULT_WS2812_PIN };
  uint32_t lastChange = 0;
  uint8_t step = 0;
  bool running = false;

public:
  void begin();
  void showLed(Color color);
  void connectHandle();
  void update();
  [[nodiscard]] bool isSignaling() const { return running; }
};
