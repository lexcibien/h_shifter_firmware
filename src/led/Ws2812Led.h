#pragma once

#include <cstdint>
#include <hardware/pio.h>

class Ws2812Led {
public:
  explicit Ws2812Led(uint pin) : pin_(pin) {}
  ~Ws2812Led();

  Ws2812Led(const Ws2812Led&) = delete;
  Ws2812Led& operator=(const Ws2812Led&) = delete;

  [[nodiscard]] bool begin();
  void show(uint8_t red, uint8_t green, uint8_t blue);
  void clear();

private:
  static constexpr uint32_t kBitRate = 800'000;
  static constexpr uint32_t kFrameBits = 24;
  static constexpr uint32_t kResetTimeUs = 50;

  void waitForReset() const;
  void releasePio();

  const uint pin_;
  PIO pio_ = nullptr;
  uint stateMachine_ = 0;
  uint programOffset_ = 0;
  uint64_t lastFrameUs_ = 0;
};
