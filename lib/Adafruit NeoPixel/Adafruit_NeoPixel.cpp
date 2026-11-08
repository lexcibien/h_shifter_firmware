/*!
 * @file Adafruit_NeoPixel.cpp
 *
 * RP2040/Pico SDK port of the Adafruit NeoPixel library.
 *
 * Written by Phil "Paint Your Dragon" Burgess for Adafruit Industries,
 * with contributions by the open source community.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "Adafruit_NeoPixel.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include <hardware/gpio.h>
#include <rp2040_pio.h>

bool isValidPicoPin(int16_t pin) { return pin >= 0 && pin < static_cast<int16_t>(NUM_BANK0_GPIOS); }

Adafruit_NeoPixel::Adafruit_NeoPixel(uint16_t n, int16_t p, neoPixelType t)
    : is800KHz(true), begun(false), numLEDs(0), numBytes(0), pin(-1), brightness(0), rOffset(1), gOffset(0), bOffset(2), wOffset(1), endTime(0) {
  updateType(t);
  updateLength(n);
  setPin(p);
}

Adafruit_NeoPixel::Adafruit_NeoPixel()
    : is800KHz(true), begun(false), numLEDs(0), numBytes(0), pin(-1), brightness(0), rOffset(1), gOffset(0), bOffset(2), wOffset(1), endTime(0) {}

Adafruit_NeoPixel::~Adafruit_NeoPixel() {
  rp2040releasePIO();
  if (isValidPicoPin(pin)) {
    gpio_deinit(static_cast<uint>(pin));
  }
}

bool Adafruit_NeoPixel::begin() {
  rp2040releasePIO();

  if (!isValidPicoPin(pin)) {
    begun = false;
    return false;
  }

  gpio_init(static_cast<uint>(pin));
  gpio_set_dir(static_cast<uint>(pin), GPIO_OUT);
  gpio_put(static_cast<uint>(pin), false);

  begun = rp2040claimPIO();
  return begun;
}

void Adafruit_NeoPixel::show() {
  if (!begun || numBytes == 0 || !pio) {
    return;
  }

  while (!canShow()) {
    tight_loop_contents();
  }

  rp2040Show(pixels, numBytes);
  endTime = time_us_32();
}

void Adafruit_NeoPixel::setPin(int16_t p) {
  if (p == pin) {
    return;
  }

  const bool restart = begun;
  rp2040releasePIO();
  if (isValidPicoPin(pin)) {
    gpio_deinit(static_cast<uint>(pin));
  }

  pin = p;
  begun = false;
  if (restart) {
    begin();
  }
}

void Adafruit_NeoPixel::updateLength(uint16_t n) {
  std::memset(pixels, 0, sizeof(pixels));
  numLEDs = 0;
  numBytes = 0;

  if (n != 1) {
    return;
  }

  numLEDs = 1;
  numBytes = (wOffset == rOffset) ? 3U : 4U;
}

void Adafruit_NeoPixel::updateType(neoPixelType t) {
  const bool oldThreeBytesPerPixel = (wOffset == rOffset);
  const bool oldSpeed = is800KHz;

  wOffset = (t >> 6U) & 0b11;
  rOffset = (t >> 4U) & 0b11;
  gOffset = (t >> 2U) & 0b11;
  bOffset = t & 0b11U;
  is800KHz = (t & NEO_KHZ400) == 0;

  if (numLEDs != 0 && oldThreeBytesPerPixel != (wOffset == rOffset)) {
    updateLength(numLEDs);
  }

  if (begun && oldSpeed != is800KHz) {
    rp2040releasePIO();
    begun = rp2040claimPIO();
  }
}

void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if (n >= numLEDs) {
    return;
  }

  if (brightness) {
    r = (r * brightness) >> 8;
    g = (g * brightness) >> 8;
    b = (b * brightness) >> 8;
  }

  uint8_t* pixel = &pixels[n * ((wOffset == rOffset) ? 3U : 4U)];
  if (wOffset != rOffset) {
    pixel[wOffset] = 0;
  }
  pixel[rOffset] = r;
  pixel[gOffset] = g;
  pixel[bOffset] = b;
}

void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  if (n >= numLEDs) {
    return;
  }

  if (brightness) {
    r = (r * brightness) >> 8;
    g = (g * brightness) >> 8;
    b = (b * brightness) >> 8;
    w = (w * brightness) >> 8;
  }

  uint8_t* pixel = &pixels[n * ((wOffset == rOffset) ? 3U : 4U)];
  if (wOffset != rOffset) {
    pixel[wOffset] = w;
  }
  pixel[rOffset] = r;
  pixel[gOffset] = g;
  pixel[bOffset] = b;
}

void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint32_t color) {
  setPixelColor(n, static_cast<uint8_t>(color >> 16U), static_cast<uint8_t>(color >> 8U), static_cast<uint8_t>(color),
                static_cast<uint8_t>(color >> 24U));
}

void Adafruit_NeoPixel::fill(uint32_t color, uint16_t first, uint16_t count) {
  if (first >= numLEDs) {
    return;
  }

  const uint32_t requestedEnd = count ? static_cast<uint32_t>(first) + count : numLEDs;
  const uint16_t end = static_cast<uint16_t>(std::min<uint32_t>(requestedEnd, static_cast<uint32_t>(numLEDs)));
  for (uint16_t i = first; i < end; ++i) {
    setPixelColor(i, color);
  }
}

uint32_t Adafruit_NeoPixel::getPixelColor(uint16_t n) const {
  if (n >= numLEDs) {
    return 0;
  }

  const uint8_t* pixel = &pixels[n * ((wOffset == rOffset) ? 3U : 4U)];
  auto const restoreBrightness = [this](uint8_t value) -> uint32_t { return brightness ? (static_cast<uint32_t>(value) << 8) / brightness : value; };

  const uint32_t red = restoreBrightness(pixel[rOffset]);
  const uint32_t green = restoreBrightness(pixel[gOffset]);
  const uint32_t blue = restoreBrightness(pixel[bOffset]);
  const uint32_t white = (wOffset == rOffset) ? 0 : restoreBrightness(pixel[wOffset]);
  return (white << 24U) | (red << 16U) | (green << 8U) | blue;
}

void Adafruit_NeoPixel::setBrightness(uint8_t value) {
  const uint8_t newBrightness = value + 1;
  if (newBrightness == brightness) {
    return;
  }

  const uint8_t oldBrightness = brightness - 1;
  uint16_t scale = 0;
  if (oldBrightness != 0) {
    scale = value == 255 ? 65535U / oldBrightness : ((static_cast<uint16_t>(newBrightness) << 8) - 1) / oldBrightness;
  }

  for (uint16_t i = 0; i < numBytes; ++i) {
    pixels[i] = (pixels[i] * scale) >> 8;
  }
  brightness = newBrightness;
}

uint8_t Adafruit_NeoPixel::getBrightness() const { return brightness - 1; }

void Adafruit_NeoPixel::clear() {
  std::memset(pixels, 0, numBytes);
}

uint32_t Adafruit_NeoPixel::ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;

  hue = ((hue * 1530L) + 32768) / 65536;
  if (hue < 510) {
    blue = 0;
    if (hue < 255) {
      red = 255;
      green = hue;
    } else {
      red = 510 - hue;
      green = 255;
    }
  } else if (hue < 1020) {
    red = 0;
    if (hue < 765) {
      green = 255;
      blue = hue - 510;
    } else {
      green = 1020 - hue;
      blue = 255;
    }
  } else if (hue < 1530) {
    green = 0;
    if (hue < 1275) {
      red = hue - 1020;
      blue = 255;
    } else {
      red = 255;
      blue = 1530 - hue;
    }
  } else {
    red = 255;
    green = 0;
    blue = 0;
  }

  const uint32_t valueScale = 1U + val;
  const uint16_t saturationScale = 1U + sat;
  const uint8_t saturationOffset = 255U - sat;
  return ((((((red * saturationScale) >> 8) + saturationOffset) * valueScale) & 0xFF00U) << 8) |
         (((((green * saturationScale) >> 8) + saturationOffset) * valueScale) & 0xFF00U) |
         (((((blue * saturationScale) >> 8) + saturationOffset) * valueScale) >> 8);
}

uint32_t Adafruit_NeoPixel::gamma32(uint32_t color) {
  auto* component = reinterpret_cast<uint8_t*>(&color);
  for (uint8_t i = 0; i < 4; ++i) {
    component[i] = gamma8(component[i]);
  }
  return color;
}

void Adafruit_NeoPixel::rainbow(uint16_t first_hue, int8_t reps, uint8_t saturation, uint8_t brightness, bool gammify) {
  for (uint16_t i = 0; i < numLEDs; ++i) {
    const auto hue = static_cast<uint16_t>(first_hue + ((static_cast<int64_t>(i) * reps * 65536) / numLEDs));
    uint32_t color = ColorHSV(hue, saturation, brightness);
    if (gammify) {
      color = gamma32(color);
    }
    setPixelColor(i, color);
  }
}

neoPixelType Adafruit_NeoPixel::str2order(const char* value) {
  int8_t red = 0;
  int8_t green = 0;
  int8_t blue = 0;
  int8_t white = -1;

  if (value) {
    for (uint8_t i = 0; value[i] != '\0'; ++i) {
      const char component = static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
      if (component == 'r') {
        red = i;
      } else if (component == 'g') {
        green = i;
      } else if (component == 'b') {
        blue = i;
      } else if (component == 'w') {
        white = i;
      }
    }
  }

  red &= 3;
  if (white < 0) {
    white = red;
  }
  return static_cast<neoPixelType>((white << 6) | (red << 4) | ((green & 3) << 2) | (blue & 3));
}

bool Adafruit_NeoPixel::rp2040claimPIO() {
  pio = nullptr;
  if (!pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &pio_sm, &pio_program_offset, static_cast<uint>(pin), 1, true)) {
    pio = nullptr;
    pio_sm = 0;
    pio_program_offset = 0;
    return false;
  }

  const float frequency = is800KHz ? 800000.0F : 400000.0F;
  ws2812_program_init(pio, pio_sm, pio_program_offset, static_cast<uint>(pin), frequency, 8);
  return true;
}

void Adafruit_NeoPixel::rp2040releasePIO() {
  if (!pio) {
    return;
  }

  pio_sm_set_enabled(pio, pio_sm, false);
  pio_remove_program_and_unclaim_sm(&ws2812_program, pio, pio_sm, pio_program_offset);
  pio = nullptr;
  pio_sm = 0;
  pio_program_offset = 0;
}

void Adafruit_NeoPixel::rp2040Show(uint8_t* pixels, uint32_t numBytes) {
  while (numBytes--) {
    pio_sm_put_blocking(pio, pio_sm, static_cast<uint32_t>(*pixels++) << 24U);
  }
}
