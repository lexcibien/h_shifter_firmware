#include "DeviceConfig.h"

namespace {
void writeU16(std::span<uint8_t> bytes, size_t offset, uint16_t value) {
  bytes[offset] = static_cast<uint8_t>(value);
  bytes[offset + 1] = static_cast<uint8_t>(value >> 8U);
}

void writeU32(std::span<uint8_t> bytes, size_t offset, uint32_t value) {
  for (size_t index = 0; index < sizeof(value); ++index) {
    bytes[offset + index] = static_cast<uint8_t>(value >> (index * 8U));
  }
}

uint16_t readU16(std::span<const uint8_t> bytes, size_t offset) {
  return static_cast<uint16_t>(bytes[offset]) | (static_cast<uint16_t>(bytes[offset + 1]) << 8U);
}

uint32_t readU32(std::span<const uint8_t> bytes, size_t offset) {
  uint32_t value = 0;
  for (size_t index = 0; index < sizeof(value); ++index) {
    value |= static_cast<uint32_t>(bytes[offset + index]) << (index * 8U);
  }
  return value;
}
} // namespace

bool DeviceConfig::isValid() const {
  if (forwardGearCount < 4 || forwardGearCount > MAX_FORWARD_GEARS) {
    return false;
  }
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  if (defaultSequentialMode) {
    return false;
  }
#endif
  return debounceMs >= 5 && debounceMs <= 50 && sequentialHoldMs >= 250 && sequentialHoldMs <= 3000 && handleDetectThreshold >= 500 &&
         handleDetectThreshold <= 4000 && hModeColor <= 0xFFFFFF && sequentialModeColor <= 0xFFFFFF;
}

std::array<uint8_t, DeviceConfig::WIRE_SIZE> DeviceConfig::encode() const {
  std::array<uint8_t, WIRE_SIZE> bytes{};
  bytes[0] = SCHEMA_VERSION;
  bytes[1] = forwardGearCount;
  bytes[2] = static_cast<uint8_t>(defaultSequentialMode);
  bytes[3] = static_cast<uint8_t>(reverseEnabled);
  writeU16(bytes, 4, debounceMs);
  writeU16(bytes, 6, sequentialHoldMs);
  writeU16(bytes, 8, handleDetectThreshold);
  writeU32(bytes, 12, hModeColor);
  writeU32(bytes, 16, sequentialModeColor);
  return bytes;
}

bool DeviceConfig::decode(std::span<const uint8_t> bytes, DeviceConfig& result) {
  if (bytes.size() != WIRE_SIZE || bytes[0] != SCHEMA_VERSION || bytes[2] > 1 || bytes[3] > 1 || bytes[10] != 0 || bytes[11] != 0) {
    return false;
  }

  DeviceConfig decoded;
  decoded.forwardGearCount = bytes[1];
  decoded.defaultSequentialMode = bytes[2] != 0;
  decoded.reverseEnabled = bytes[3] != 0;
  decoded.debounceMs = readU16(bytes, 4);
  decoded.sequentialHoldMs = readU16(bytes, 6);
  decoded.handleDetectThreshold = readU16(bytes, 8);
  decoded.hModeColor = readU32(bytes, 12);
  decoded.sequentialModeColor = readU32(bytes, 16);
  if (!decoded.isValid()) {
    return false;
  }
  result = decoded;
  return true;
}
