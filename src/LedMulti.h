#pragma once

#include <Adafruit_NeoPixel.h>

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
  inline static bool blockSignal = false;

  void begin();
  void showLed(Color color);
  void connectHandle();
  void update();
};
