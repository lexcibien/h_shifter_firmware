#pragma once

#include "ShifterProject.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

struct DeviceConfig {
  static constexpr uint8_t SCHEMA_VERSION = 1;
  static constexpr uint16_t DEFAULT_DEBOUNCE_MS = 15;
  static constexpr uint16_t DEFAULT_SEQUENTIAL_HOLD_MS = 1000;
  static constexpr uint16_t DEFAULT_HANDLE_THRESHOLD = 3430;
  static constexpr uint32_t DEFAULT_H_COLOR = 0xFF8000;
  static constexpr uint32_t DEFAULT_SEQUENTIAL_COLOR = 0xFF00FF;
  static constexpr size_t WIRE_SIZE = 20;

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  static constexpr uint8_t MAX_FORWARD_GEARS = 6;
#else
  static constexpr uint8_t MAX_FORWARD_GEARS = 7;
#endif

  // NOLINTBEGIN (misc-non-private-member-variables-in-classes)
  uint8_t forwardGearCount = MAX_FORWARD_GEARS;
  bool defaultSequentialMode = false;
  bool reverseEnabled = true;
  uint16_t debounceMs = DEFAULT_DEBOUNCE_MS;
  uint16_t sequentialHoldMs = DEFAULT_SEQUENTIAL_HOLD_MS;
  uint16_t handleDetectThreshold = DEFAULT_HANDLE_THRESHOLD;
  uint32_t hModeColor = DEFAULT_H_COLOR;
  uint32_t sequentialModeColor = DEFAULT_SEQUENTIAL_COLOR;
  // NOLINTEND
  static constexpr DeviceConfig defaults() { return {}; }
  [[nodiscard]] bool isValid() const;
  [[nodiscard]] std::array<uint8_t, WIRE_SIZE> encode() const;
  static bool decode(std::span<const uint8_t> bytes, DeviceConfig& result);

  bool operator==(const DeviceConfig&) const = default;
};
