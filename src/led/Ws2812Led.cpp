#include "Ws2812Led.h"
#include "ws2812.pio.h"
#include <hardware/clocks.h>
#include <pico/time.h>

Ws2812Led::~Ws2812Led() { releasePio(); }

bool Ws2812Led::begin() {
  if (pio_ != nullptr) {
    return true;
  }

  if (!pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio_, &stateMachine_, &programOffset_, pin_, 1, true)) {
    return false;
  }

  ws2812_program_init(pio_, stateMachine_, programOffset_, pin_, static_cast<float>(kBitRate), false);
  return true;
}

void Ws2812Led::show(uint8_t red, uint8_t green, uint8_t blue) {
  if (pio_ == nullptr) {
    return;
  }

  waitForReset();

  // WS2812 accepts the most significant bit first, in GRB byte order.
  const uint32_t grb = (static_cast<uint32_t>(green) << 16U) | (static_cast<uint32_t>(red) << 8U) | static_cast<uint32_t>(blue);
  pio_sm_put_blocking(pio_, stateMachine_, grb << 8U);
  lastFrameUs_ = time_us_64();
}

void Ws2812Led::clear() { show(0, 0, 0); }

void Ws2812Led::waitForReset() const {
  if (lastFrameUs_ == 0) {
    return;
  }

  // A 24-bit frame takes 30 us at 800 kHz. Keep the line low for at least
  // the WS2812 reset interval before queuing the next frame.
  constexpr uint64_t frameTimeUs = (1'000'000ULL * kFrameBits) / kBitRate;
  const uint64_t nextFrameUs = lastFrameUs_ + frameTimeUs + kResetTimeUs;
  const uint64_t nowUs = time_us_64();
  if (nowUs < nextFrameUs) {
    busy_wait_until(from_us_since_boot(nextFrameUs));
  }
}

void Ws2812Led::releasePio() {
  if (pio_ == nullptr) {
    return;
  }

  pio_sm_set_enabled(pio_, stateMachine_, false);
  pio_remove_program_and_unclaim_sm(&ws2812_program, pio_, stateMachine_, programOffset_);
  pio_ = nullptr;
}
